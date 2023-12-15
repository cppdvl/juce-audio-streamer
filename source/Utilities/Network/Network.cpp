//
// Created by Julian Guarin on 3/12/23.
//

#include "uvgRTP.h"
#include "Utilities.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <random>
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
    std::vector<std::string> getNetworkInterfaces()
    {
        std::vector<std::string> entries{};
        auto info = getNetworkInfo();
        for (auto& [family, interfaces] : info)
        {
            for (auto& [interface, address] : interfaces)
            {
                if (family == "ipv4") entries.push_back(family+":"+interface+":"+address);
            }
        }
        return entries;
    }


    void createSession(SPRTP spRTP,
        uint64_t& sessionID,
        uint64_t& streamOutID,
        uint64_t& streamInID,
        int& outPort,
        int& inPort,
        std::string ip)
    {
        if (sessionID) return; //This is already created.
        sessionID = spRTP->CreateSession(ip);
        if (!sessionID)
        {
            std::cout << "Failed to create session." << std::endl;
            return;
        }
        streamInID = createInStream(spRTP, sessionID, streamInID, outPort, inPort);
    }
    void createSession();
    uint64_t createOutStream(SPRTP spRTP,
        uint64_t sessionID,
        uint64_t& streamOutID,
        int& outPort)
    {
        if (!sessionID) return (uint64_t )0;
        streamOutID = spRTP->CreateStream(sessionID, outPort, 0);
        return streamOutID;
    }

    uint64_t createInStream(SPRTP spRTP,
        uint64_t sessionID,
        uint64_t& streamInID,
        int& outPort,
        int& inPort)
    {
        //THIS FUNCTION SHOULD BE REPLACED WITH SOMETHING CALLED GET SESSION.
        //That function will.
        //Will ask Maui for access.
        //Once Maui has provided access the function will provide streaming information.
        //The Function will start listening.

        std::vector<int32_t> ports{8888, 8889};
        bool searchingForListeningPorts = true;
        for (auto port : ports)
        {
            inPort = port;
            outPort = 8888 + (port + 1) % 2;

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(1, 5);
            std::this_thread::sleep_for(std::chrono::seconds(dis(gen)));

            //check if port is in use or we are looking
            if (!searchingForListeningPorts) break;
            if (RTPWrapUtils::udpPortIsInUse(port)) continue;

            //attempt to create an input stream
            streamInID = spRTP->CreateStream(sessionID, port, 1);
            auto pStream = UVGRTPWrap::GetSP(spRTP)->GetStream(streamInID);
            jassert(pStream && streamInID);
            {
                std::cout << "Inbound Stream ID: " << streamInID << std::endl;
                searchingForListeningPorts = false;
                /* CONTINUE HERE => INSTALL RECEIVER HOOK: THE RECEIVER HOOK SHOULD BE IN THE UTILITIES UNIT*/

            }
        }
    }
}

