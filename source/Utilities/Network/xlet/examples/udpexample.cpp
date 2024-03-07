//
// Created by Julian Guarin on 10/01/24.
//
#include "xlet.h"
#include <iomanip>
#include <sstream>





void log(std::string letId, const std::string& msg)
{
    std::cout << "[ " << letId << " ] : " << msg << std::endl;
}

void printHexy(std::vector<std::byte>&data, int maxrowsize = 16, int groupsize = 4)
{
    int rowsize = 0;
    int address = 0;
    int digitsInAddress = XLET_MAXBLOCKSIZE_SHIFTER % groupsize == 0 ? (XLET_MAXBLOCKSIZE_SHIFTER / groupsize) : (XLET_MAXBLOCKSIZE_SHIFTER / groupsize) + 1;

    auto originalSize = data.size();
    while (data.size() % maxrowsize) data.push_back(std::byte{0});

    std::vector<char> digits{};
    for (auto& byte : data)
    {

        if (!(rowsize % maxrowsize)) {
            digits.clear();
            std::cout << std::hex << std::setw(digitsInAddress) << std::setfill('0') << (int)address << " ";
        }

        if (rowsize % 4 == 0)
        {
            std::cout << " ";
        }
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (unsigned long long)byte;
        digits.push_back((char)byte);

        if ((rowsize % maxrowsize) == (maxrowsize - 1))
        {
            std::cout << " ";
            for (auto& digit : digits)
            {
                if (std::isprint(digit))
                {
                    std::cout << digit;
                }
                else
                {
                    std::cout << ".";
                }
            }
            std::cout << std::endl;
            address += maxrowsize;
        }
        else if ((rowsize % groupsize) == (groupsize - 1))
        {
            std::cout << " ";
        }
        else
        {
            std::cout << ".";
        }
        rowsize++;

    }
    std::cout << std::endl;
    //go back to original size
    data.resize(originalSize);
}
void handleServerIncomingData(uint64_t peerId, std::vector<std::byte>& data) {
    // Process incoming data
    log(xlet::UDPlet::letIdToIpString(peerId), "Received data of size: " + std::to_string(data.size()));
    printHexy(data);
}

void runServer(const std::string& ip, int port)
{
    xlet::UDPInOut udpServer(ip, 8899, true);

    udpServer.letBindedOn.Connect(+[](uint64_t letId, std::thread::id id){
        std::stringstream ss; ss << "UDP Server binded and waiting for data on thread: " << id;
        log(xlet::UDPlet::letIdToString(letId) , ss.str());
    });

    udpServer.letDataFromPeerIsReady.Connect(handleServerIncomingData);
    //std::thread inboundDataHandlerThread(udpServer.inboundDataHandlerFunc);

    if (udpServer.valid()) {
        std::cout << "UDP Server is running using socket: " << ip << std::endl;
    } else {
        std::cerr << "Failed to initialize UDP Server" << std::endl;
    }
    //inboundDataHandlerThread.join();
}

void runClient(const std::string& ip, int port)
{
    xlet::UDPOut udpClient(ip, 8899); //This will point to the server.
    if (udpClient.valid()) {
        std::cout << "UDP Client is running"<< std::endl;

        std::string s("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
        std::vector<std::byte> data{};
        for (auto& c : s)
        {
            data.push_back(std::byte(c));
        }
        udpClient.pushData(data);
    } else {
        std::cerr << "Failed to initialize UDP Client" << std::endl;
    }
}
int main(int argc, char**argv)
{

    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <server|client>" << std::endl;
        return 1;
    }
    std::string mode = argv[1];
    if (mode == "server")
    {
        runServer("127.0.0.1", 8899);
    }
    else if (mode == "client")
    {
        runClient("127.0.0.1", 8899);
    }
    else
    {
        std::cerr << "Usage: " << argv[0] << " <server|client>" << std::endl;
        return 1;
    }
    return 0;
}
