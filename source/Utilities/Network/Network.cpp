//
// Created by Julian Guarin on 3/12/23.
//

#include "Utilities.h"

#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
namespace Utilities::Network
{

    std::map<std::string, std::map<std::string, std::string>> getNetworkInfo()
    {
        struct ifaddrs* ifs = nullptr;
        struct ifaddrs* tmp = nullptr;
        auto success        = getifaddrs(&ifs) == 0;

        std::map<std::string, std::map<std::string, std::string>> info{};
        if (!success) return info;

        tmp = ifs;
        std::array<char, INET_ADDRSTRLEN> bufferIPV4;
        std::array<char, INET6_ADDRSTRLEN> bufferIPV6;

        while (tmp)
        {
            if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
            {
                inet_ntop(AF_INET, &(reinterpret_cast<struct sockaddr_in*>(tmp->ifa_addr)->sin_addr), bufferIPV4.data(), INET_ADDRSTRLEN);
                info["ipv4"][tmp->ifa_name] = bufferIPV4.data();
            }
            else
            {
                inet_ntop(AF_INET6, &(reinterpret_cast<struct sockaddr_in6*>(tmp->ifa_addr)->sin6_addr), bufferIPV6.data(), INET6_ADDRSTRLEN);
                info["ipv6"][tmp->ifa_name] = bufferIPV6.data();
            }
            tmp = tmp->ifa_next;
        }
        return info;

    }
}

