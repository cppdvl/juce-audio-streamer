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
    return static_cast<uint32_t>(peerId & 0xFFFF);
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
xlet::Direction direction,
bool theLetListens
)
{
    this->direction = direction;
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
        bytesSent += bytesSentNow;
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
                if (!qout_.empty()){
                    std::lock_guard<std::mutex> lock(sockMutex);
                    auto data = qout_.front();
                    qout_.pop();
                    letDataReadyToBeTransmitted.Emit(letIdToString(data.first), data.second);
                    pushData(data.first, data.second);
                }
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
                std::lock_guard<std::mutex> lock(sockMutex);
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
                inDataBuffer.resize(n);
                {
                    std::lock_guard<std::mutex> lock(sockMutex);
                    if (queueManaged) qin_.push(xlet::Data(sockAddToPeerId(cliaddr), inDataBuffer));
                    else letDataFromPeerIsReady.Emit(sockAddToPeerId(cliaddr), inDataBuffer);
                }
            }
        }
    }};
    if (queueManaged)
    {
        qThread = std::thread{[this](){
            while (qPause && sockfd_ > 0);
            while (sockfd_ > 0) {
                if (qin_.empty()){
                    std::lock_guard<std::mutex> lock(sockMutex);
                    auto data = qin_.front();
                    qin_.pop();
                    letDataFromPeerIsReady.Emit(data.first, data.second);
                }
            }
        }};
    }

}
/********************/
/****** UDPInOut *****/
xlet::UDPInOut::UDPInOut(const std::string ipstring, int port, bool listen, bool qSynced, bool loopback) : UDPlet(ipstring, port, direction, listen)
{
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
                    std::lock_guard<std::mutex> lock(sockMutex);
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
                    inDataBuffer.resize(n);
                    {
                        std::lock_guard<std::mutex> lock(sockMutex);
                        if (queueManaged) qin_.push(xlet::Data(sockAddToPeerId(cliaddr), inDataBuffer));
                        else letDataFromPeerIsReady.Emit(sockAddToPeerId(cliaddr), inDataBuffer);
                    }
                }
            }

        }};
    if (queueManaged)
    {
        qThread = std::thread{[this, loopback](){
            while (qPause);
            letThreadStarted.Emit(static_cast<uint64_t>(sockfd_));
            while (sockfd_ > 0) {
                if  (!qin_.empty())
                {
                    std::lock_guard<std::mutex> lock(sockMutex);
                    auto data = qin_.front();
                    qin_.pop();
                    letDataFromPeerIsReady.Emit(data.first, data.second);
                }
                if (!qout_.empty())
                {
                    std::lock_guard<std::mutex> lock(sockMutex);
                    auto data = qout_.front();
                    qout_.pop();
                    letDataReadyToBeTransmitted.Emit(letIdToString(data.first), data.second);
                    if (!loopback)
                    {
                        pushData(data.first, data.second);
                    }
                    else
                    {
                        qin_.push(data);
                    }
                }
            }
        }};
    }

}


