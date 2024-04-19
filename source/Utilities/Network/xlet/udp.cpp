#include "xlet.h"
#include <arpa/inet.h>


struct sockaddr_in xlet::UDPlet::toSystemSockAddr(std::string ip, int port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    //inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    return addr;
}

uint64_t xlet::UDPlet::sockAddrIpToUInt64(struct sockaddr_in& addr)
{
    uint64_t ip = *reinterpret_cast<uint32_t*>(&addr.sin_addr.s_addr);
    ip          <<= 32;
    return ip;
}

uint64_t xlet::UDPlet::sockAddrPortToUInt64(struct sockaddr_in& addr)
{
    return static_cast<uint64_t>(ntohs(addr.sin_port));
}

uint64_t xlet::UDPlet::sockAddToPeerId(struct sockaddr_in& addr)
{
    return sockAddrIpToUInt64(addr) | sockAddrPortToUInt64(addr);
}

struct sockaddr_in xlet::UDPlet::peerIdToSockAddr(uint64_t peerId)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint32_t>(peerId & 0xFFFF));
    auto ipString = letIdToIpString(peerId);
    addr.sin_addr.s_addr = inet_addr(ipString.c_str());
    return addr;
}

std::string xlet::UDPlet::letIdToString(uint64_t peerId)
{
    std::stringstream ss; ss << letIdToIpString(peerId);
    ss << ":" << peerIdToPort(peerId);
    return ss.str();
}

int xlet::UDPlet::peerIdToPort(uint64_t peerId)
{
    return static_cast<int32_t>(peerId & 0xFFFF);
}

std::string xlet::UDPlet::letIdToIpString(uint64_t peerId)
{
    std::stringstream ss;
    ss << (((peerId >> 32) >> 0x00) & 0xFF) << ".";
    ss << (((peerId >> 32) >> 0x08) & 0xFF) << ".";
    ss << (((peerId >> 32) >> 0x10) & 0xFF) << ".";
    ss << (((peerId >> 32) >> 0x18) & 0xFF);
    return ss.str();
}

xlet::UDPlet::UDPlet(
const std::string ipstring,
int port,
xlet::Direction edirection,
bool theLetListens
)
{
    this->direction = edirection;
    servaddr_       = toSystemSockAddr(ipstring, port);
    servId_         = sockAddToPeerId(servaddr_);

    if ( direction == xlet::Direction::INB && theLetListens == false)
    {
        theLetListens = true;
    }
    else if ( direction == xlet::Direction::OUTB && theLetListens == true)
    {
        theLetListens = false;
    }

    if ((sockfd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        sockfd_ = -1;
        return;
    }

    if (fcntl(sockfd_, F_SETFL, fcntl(sockfd_, F_GETFL, 0) | O_NONBLOCK) == -1)
    {
        letOperationalError.Emit(sockfd_, "fcntl");
        close(sockfd_);
        sockfd_ = -1;
        return;
    }

    if (theLetListens)
    {
        if (bind(sockfd_, (struct sockaddr *)&servaddr_, sizeof(servaddr_)) < 0)
        {
            letOperationalError.Emit(sockfd_, "bind");
            close(sockfd_);
            sockfd_ = -1;
            return;
        }
    }
}

std::size_t xlet::UDPlet::pushData(const std::vector<std::byte>& data) {
    return pushData(servId_, data);
}
std::size_t xlet::UDPlet::pushData(const uint64_t peerId,  const std::vector<std::byte>& data) {
    if (sockfd_ < 0) {
        //TODO: Trigger a critical error signal
        letInvalidSocketError.Emit();
        return 0;
    }

    auto addr = peerIdToSockAddr(peerId);
    auto bytesSent = static_cast<size_t>(0);
    while (bytesSent < data.size()) {
        auto bytesSentNow = sendto(sockfd_, data.data() + bytesSent, data.size() - bytesSent, 0, (struct sockaddr *) &addr, sizeof(addr));
        if (bytesSentNow < 0) {
            //Trigger a critical error signal
            auto errorMessage = strerror(errno);
            letOperationalError.Emit(sockfd_, errorMessage);
            return 0;
        }
        bytesSent += static_cast<size_t>(bytesSentNow);
    }
    return bytesSent;
}



/********************/
/****** UDPOut ******/
xlet::UDPOut::UDPOut(const std::string ipstring, int port, bool qSynced) : UDPlet(ipstring, port, xlet::Direction::OUTB, false)
{

    if (qSynced)
    {
        queueManaged = true;
        qThread = std::thread{[this](){
            while (qPause  && sockfd_ > 0);
            const std::string peerIdString = letIdToString(sockAddToPeerId(servaddr_));
            letThreadStarted.Emit(static_cast<uint64_t>(sockfd_));
            while (sockfd_ > 0) {

                if (qout_.empty()) continue;

                std::unique_lock<std::mutex> lock(mtxout_);
                Data data = qout_[0];
                qout_.erase(qout_.begin(), qout_.begin() + 1);
                lock.unlock();

                letDataReadyToBeTransmitted.Emit(letIdToString(data.first), data.second);
                pushData(data.first, data.second);
            }
        }};
    }
}
/********************/
/******** UDPIn ******/
xlet::UDPIn::UDPIn(const std::string ipstring, int port, bool qSynced) : UDPlet(ipstring, port, xlet::Direction::INB, true)
{
    queueManaged = qSynced;
    recvThread = std::thread{[this](){
        while (qPause && sockfd_ > 0);
        letBindedOn.Emit(servId_, std::this_thread::get_id());
        letThreadStarted.Emit(static_cast<uint64_t>(sockfd_));
        while (sockfd_ > 0) {
            struct sockaddr_in cliaddr;
            socklen_t len = sizeof(cliaddr);
            std::vector<std::byte> inDataBuffer(XLET_MAXBLOCKSIZE, std::byte{0});
            ssize_t n = 0;
            {
                n = recvfrom(sockfd_, inDataBuffer.data(), inDataBuffer.size(), 0, (struct sockaddr *) &cliaddr, &len);
            }

            if (n < 0) {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    letOperationalError.Emit(sockfd_, "recvfrom");
                    continue;
                }
            }
            else if (n > 0)
            {
                inDataBuffer.resize(static_cast<size_t>(n));

                if (queueManaged)
                {
                    std::unique_lock<std::mutex> lock(mtxin_);
                    qin_.push_back (xlet::Data { .first = sockAddToPeerId (cliaddr), .second = inDataBuffer });
                    lock.unlock();
                }
                else
                {
                    letDataFromPeerIsReady.Emit(sockAddToPeerId(cliaddr), inDataBuffer);
                }

            }
        }
    }};
    if (queueManaged)
    {
        qThread = std::thread{[this](){
            while (qPause && sockfd_ > 0);
            while (sockfd_ > 0) {

                if (qin_.empty()) continue;

                std::unique_lock<std::mutex> lock(mtxin_);
                auto data = qin_[0];
                qin_.erase(qin_.begin(), qin_.begin() + 1);
                lock.unlock();

                letDataFromPeerIsReady.Emit(data.first, data.second);
            }
        }};
    }

}
/********************/
/****** UDPInOut *****/
xlet::UDPInOut::UDPInOut(const std::string ipstring, int port, bool listen, bool qSynced, bool loopback) : UDPlet(ipstring, port, xlet::Direction::INOUTB, listen)
{
    std::cout << "Creating the UDPInOut" << std::endl;
    std::cout << "UDP Let Socket no. " << sockfd_ << std::endl;
    std::cout << "UDP Let Direction " << direction << std::endl;
    std::cout << "UDP Let Listens " << listen << std::endl;
    std::cout << "UDP Let qSynced " << qSynced << std::endl;
    std::cout << "Loopback " << loopback << std::endl;
    std::cout << "Calling IP: " << ipstring << std::endl;
    std::cout << "Port: " << port << std::endl;


    if (loopback && sockfd_ > 0)
    {
        close(sockfd_);
    }
    queueManaged = qSynced;
    if (!loopback) recvThread = std::thread{[this](){
            while (qPause && sockfd_ > 0);
            if (sockfd_ < 0) return;
            letBindedOn.Emit(servId_, std::this_thread::get_id());
            letThreadStarted.Emit(static_cast<uint64_t>(sockfd_));

            while (sockfd_ > 0) {
                struct sockaddr_in cliaddr;
                socklen_t len = sizeof(cliaddr);
                std::vector<std::byte> inDataBuffer(XLET_MAXBLOCKSIZE, std::byte{0});
                ssize_t n = 0;
                {
                    n = recvfrom(sockfd_, inDataBuffer.data(), inDataBuffer.size(), 0, (struct sockaddr *) &cliaddr, &len);
                }
                if (n < 0) {
                    if (errno != EWOULDBLOCK && errno != EAGAIN) {
                        letOperationalError.Emit(sockfd_, "recvfrom");
                        continue;
                    }
                }
                else if (n > 0)
                {
                    inDataBuffer.resize(static_cast<size_t>(n));
                    {
                        if (queueManaged)
                        {
                            std::unique_lock<std::mutex> lock(mtxin_);
                            push_back(xlet::Data { .first = sockAddToPeerId (cliaddr), .second = inDataBuffer }, xlet::Direction::INB);
                            lock.unlock();
                        }
                        else letDataFromPeerIsReady.Emit(sockAddToPeerId(cliaddr), inDataBuffer);
                    }
                }
            }

        }};
    recvThread.detach();
    if (queueManaged)
    {
        qThread = std::thread{[this, loopback](){
            while (qPause);
            letThreadStarted.Emit(static_cast<uint64_t>(sockfd_));
            while (sockfd_ > 0) {

                if  (!qin_.empty())
                {
                     std::unique_lock<std::mutex> lock(mtxin_);
                    auto data = qin_[0];
                    qin_.erase(qin_.begin(), qin_.begin() + 1);
                    lock.unlock();

                    letDataFromPeerIsReady.Emit(data.first, data.second);
                }

                if (!qout_.empty())
                {

                    std::unique_lock<std::mutex> lock(mtxout_);
                    Data data = qout_[0];
                    qout_.erase(qout_.begin(), qout_.begin() + 1);
                    lock.unlock();

                    std::vector<std::byte> payload = data.second;
                    if (payload.empty())
                    {
                        std::cout << "PLUGIN PRODUCED NO DATA !!!!!!!!!!!!!!!!!!!!!" << std::endl;
                        return;
                    }

                    std::string addrString = letIdToString(data.first);
                    letDataReadyToBeTransmitted.Emit(letIdToString(data.first), data.second);
                    if (!loopback)
                    {
                        pushData(data.first, data.second);
                    }
                    else
                    {
                        qin_.push_back(data);
                    }

                }
            }
        }};
        qThread.detach();
    }

}


