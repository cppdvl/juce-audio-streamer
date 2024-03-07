/*! \file uds.cpp
 *  Unix Domain Socket implementation for DAWn Audio Relay Server
 *  \author JGuarin (cppdvl) 2024
 */
#include "xlet.h"
#include <sys/un.h>



xlet::UDSlet::UDSlet(
    const std::string &sockpath,
    xlet::Direction direction,
    bool theLetListens
)
{
    this->direction = direction;

    if ( direction == xlet::Direction::INB && theLetListens == false)
    {
        theLetListens = true;
    }
    else if ( direction == xlet::Direction::OUTB && theLetListens == true)
    {
        theLetListens = false;
    }

    if ((sockfd_ = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        sockfd_ = -1;
        return;
    }
    struct sockaddr_un servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strncpy(servaddr.sun_path, sockpath.c_str(), sizeof(servaddr.sun_path)-1);

    if (theLetListens)
    {
        unlink(sockpath.c_str());

        if (bind(sockfd_, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        {
            letOperationalError.Emit(sockfd_, "bind");
            close(sockfd_);
            sockfd_ = -1;
            return;
        }

        if (listen(sockfd_, 1) < 0)
        {
            letOperationalError.Emit(sockfd_, "listen");
            close(sockfd_);
            return;
        }

        pollfds_.push_back({sockfd_, POLLIN, 0});
        inboundDataHandler = std::function<void()>{[this]() {
            auto threadId = std::this_thread::get_id();
            letIsListening.Emit(sockfd_, threadId);

            while (true) {

                if(poll(pollfds_.data(), pollfds_.size(), -1) < 0){
                    std::string pollError = strerror(errno);
                    letOperationalError.Emit(sockfd_, pollError);
                    return;
                }

                for (auto&fd : pollfds_)
                {
                    if (fd.revents & POLLIN)
                    {
                        if (fd.fd == sockfd_)
                        {
                            int connfd = accept(sockfd_, nullptr, nullptr);
                            if (connfd < 0) {
                                letOperationalError.Emit(sockfd_, "accept");
                                continue;
                            }
                            letAcceptedANewConnection.Emit(sockfd_,connfd);
                            pollfds_.push_back({connfd, POLLIN, 0});
                        }
                        else
                        {
                            std::vector<std::byte> dataInBlock(XLET_MAXBLOCKSIZE, std::byte{0});
                            ssize_t bytesRead = read(fd.fd, dataInBlock.data(), dataInBlock.size());
                            if (bytesRead <= 0) {
                                if (bytesRead) letOperationalError.Emit(fd.fd, strerror(errno));
                                letWillCloseConnection.Emit(static_cast<uint64_t >(fd.fd));
                                close (fd.fd);
                                fd.fd = -1;
                                continue;
                            }
                            dataInBlock.resize(bytesRead);
                            letDataFromPeerReady.Emit(fd.fd, dataInBlock);
                        }
                    }
                }
                pollfds_.erase(std::remove_if(pollfds_.begin(), pollfds_.end(), [](auto& fd){return fd.fd < 0;}), pollfds_.end());
            }
        }};
    }
    else if (direction == xlet::Direction::OUTB || direction == xlet::Direction::INOUTB)
    {
        if (connect(sockfd_, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        {
            return;
        }
        if (direction == xlet::Direction::OUTB)
        {
            return;
        }
        else
        {
            //DIRECTION INOUTB
            pollfds_.push_back({sockfd_, POLLIN, 0});
            inboundDataHandler = std::function<void()>{[this]() {
                while (true) {
                    if (poll(pollfds_.data(), pollfds_.size(), -1) < 0){
                        letOperationalError.Emit(sockfd_, strerror(errno));
                        return;
                    }
                    for (auto&fd : pollfds_) {
                        if (sockfd_ == fd.fd && fd.revents & POLLIN) {
                            std::vector<std::byte> dataInBlock(XLET_MAXBLOCKSIZE, std::byte{0});
                            ssize_t bytesRead = read(fd.fd, dataInBlock.data(), dataInBlock.size());
                            if (bytesRead <= 0) {
                                letWillCloseConnection.Emit(static_cast<uint64_t>(fd.fd));
                                close(fd.fd);
                                fd.fd = -1;
                                continue;
                            }
                            dataInBlock.resize(bytesRead);
                            letDataFromPeerReady.Emit(0, dataInBlock);
                        }
                    }
                    pollfds_.erase(std::remove_if(pollfds_.begin(), pollfds_.end(), [](auto& fd){return fd.fd < 0;}), pollfds_.end());
                }
            }};
        }
    }
}
std::size_t xlet::UDSlet::pushData(const std::vector<std::byte>& refData){
    return pushData(static_cast<uint64_t>(sockfd_), refData);
}

std::size_t xlet::UDSlet::pushData(const uint64_t destId, const std::vector<std::byte>& data){

    if (direction == xlet::Direction::INB) {
        letIsIINBOnly.Emit();
        return 0;
    }

    int socket = static_cast<int>(destId & 0xFFFFFFFF);
    if (socket < 0) {
        letInvalidSocketError.Emit();
        return 0;
    }

    const std::byte *dataPtr = data.data();
    std::size_t dataLen = data.size();

    //write thru socket
    while (dataLen > 0) {
        ssize_t bytesWritten = write(socket, dataPtr, dataLen);
        if (bytesWritten < 0) {
            letOperationalError.Emit(socket, strerror(errno));
            return 0;
        }
        dataPtr += bytesWritten;
        dataLen -= bytesWritten;
    }

    return data.size();
}

/********************/
/****** UDSOut ******/
xlet::UDSOut::UDSOut(const std::string &sockpath) : UDSlet(sockpath, xlet::Direction::OUTB, false)
{
}

/********************/
/****** UDSIn ******/
xlet::UDSIn::UDSIn(const std::string &sockpath) : UDSlet(sockpath, xlet::Direction::INB, true)
{
}

/********************/
/****** UDSInOut ****/
xlet::UDSInOut::UDSInOut(const std::string &sockpath, bool listen) : UDSlet(sockpath, xlet::Direction::INOUTB, listen)
{


}
