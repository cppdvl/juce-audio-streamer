//
// Created by Julian Guarin on 10/01/24.
//
#include "xlet.h"
#include <iomanip>
#include <sstream>

void log(int id, const std::string& msg)
{
    std::cout << " [" << id << "] : " << msg << std::endl;
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
void handleServerIncomingData(uint64_t connId, std::vector<std::byte>& data) {
    // Process incoming data
    log(connId, "Received data of size: " + std::to_string(data.size()));
    // Additional processing can be done here
    printHexy(data);
}


void serverMainLoop(const std::string& sockpath)
{
    xlet::UDSInOut udsServer(sockpath, true);

    udsServer.letIsListening.Connect(+[](uint64_t sockfd, std::thread::id id){
        std::stringstream ss; ss << "UDS Server is listening on thread: " << id;
        log( sockfd , ss.str());
    });
    udsServer.letWillCloseConnection.Connect(+[](uint64_t connfd){
        std::stringstream ss; ss << "Closing connection";
        log( connfd , ss.str());
    });
    udsServer.letDataFromPeerReady.Connect(handleServerIncomingData);
    udsServer.letOperationalError.Connect(+[](int sockfd, const std::string& msg){
        std::stringstream ss; ss << "Error on socket: " << sockfd << " : " << msg;
        log( sockfd , ss.str());
    });
    udsServer.letAcceptedANewConnection.Connect(+[](uint64_t sockfd, int connfd){
        std::stringstream ss; ss << "Accepted new connection: " << "[" << connfd << "]";
        log( sockfd , ss.str());
    });
    std::thread inboundDataHandlerThread(udsServer.inboundDataHandler);


    if (udsServer.valid()) {
        std::cout << "UDS Server is running using socket: " << sockpath << std::endl;
        // Server's main loop goes here
    } else {
        std::cerr << "Failed to initialize UDS Server" << std::endl;
    }

    inboundDataHandlerThread.join();
}

void clientMainLoop(const std::string& sockpath)
{
    xlet::UDSInOut udsClient(sockpath, false);
    if (udsClient.valid()) {
        std::cout << "UDS Client is running using socket: " << sockpath << std::endl;

        std::string s("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
        std::vector<std::byte> data{};
        for (auto& c : s)
        {
            data.push_back(std::byte(c));
        }
        udsClient.pushData(data);
    } else {
        std::cerr << "Failed to initialize UDS Client" << std::endl;
    }
}
int main(int argc, char**argv)
{
    std::string sockpath = "/tmp/uds_socket";

    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <server|client>" << std::endl;
        return 1;
    }
    std::string mode = argv[1];
    if (mode == "server")
    {
        serverMainLoop(sockpath);
    }
    else if (mode == "client")
    {
        clientMainLoop(sockpath);
    }
    else
    {
        std::cerr << "Usage: " << argv[0] << " <server|client>" << std::endl;
        return 1;
    }
    return 0;
}
