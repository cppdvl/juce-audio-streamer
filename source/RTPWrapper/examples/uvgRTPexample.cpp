#include "uvgRTP.h"

#include <iostream>

// TCP PORT
uint16_t STREAM_PORT = 8888;

// demonstration parameters of the example
constexpr uint32_t PAYLOAD_MAXLEN = (0xffff - 0x1000);
constexpr int TEST_PACKETS = 100;
std::string REMOTE_ADDRESS = "127.0.0.1";
int DIRECTION = 1;


void rtp_receive_hook(void* arg, uvgrtp::frame::rtp_frame* pframe)
{
    static int rcvframes = 0;
    std::cout << ++rcvframes << ": Received a frame" << std::endl;
    uvgrtp::frame::dealloc_frame(pframe);
}


int main(int argc, char** argv)
{

    //THIS WAS CREATED WITH SOLELY PURPOSE OF COMPILE CHECK THE UVGRTPWrapper made sense.
    //Of course we need to use the general Interface.
    //UVGRTPWrapper.

    // Parse command line arguments: -p {port} -a {address} -d {0: outbound | 1: inbound}
    // If no arguments are provided, the default values are used
    // The default values are: port = "8888", address = "127.0.0.1", direction = '0' (outbound)

    // Parse command line arguments.
    // If no arguments are provided, the default values are used.
    std::string REMOTE_ADDRESS = "127.0.0.1";
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "-p") {
                if (i+1 >= argc)
                {
                    std::cout << "Invalid arguments for port (-p)" << std::endl;
                    return -1;
                }
                STREAM_PORT = std::stoi(argv[i + 1]);
            }
            else if (std::string(argv[i]) == "-a") {
                if (i+1 >= argc)
                {
                    std::cout << "Invalid arguments for address (-a)" << std::endl;
                    return -1;
                }
                REMOTE_ADDRESS = std::string{argv[i + 1]};
            }
            else if (std::string(argv[i]) == "-d") {
                if (i+1 >= argc)
                {
                    std::cout << "Invalid arguments for direction (-d)" << std::endl;
                    return -1;
                }
                DIRECTION = argv[i + 1][0] == '0' ? 0 : 1;
            }
        }
    }

    //Print a summary of the configuration information:
    std::cout << "=-==============================================-=" << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "Port: " << STREAM_PORT << std::endl;
    std::cout << "Address: " << REMOTE_ADDRESS << std::endl;
    std::cout << "Direction: " << (DIRECTION == 0 ? "outbound" : "inbound") << std::endl;
    std::cout << "=-==============================================-=" << std::endl;

/****************        FACTORY      ****************/
    SPRTP pRTP = std::make_shared<UVGRTPWrap>();
    std::cout << "Wrapper Instanced" << std::endl;

/********************* GENERIC CODE TO CREATE A SESSION AND A STREAM **************************/
    // Initialize the RTP wrapper
    uint64_t wrapperHandle = pRTP->Initialize();

    // Create a session
    std::cout << "Creating a session" << std::endl;
    uint64_t sessionId = pRTP->CreateSession("127.0.0.1");

    if (!sessionId) {
        std::cout << "Failed to create a session" << std::endl;
        return -1;
    }


    // Create two streams for sending and receiving
    uint64_t streamId = pRTP->CreateStream(sessionId, STREAM_PORT, DIRECTION);  //0: 0utput


    auto tearDown = [streamId, sessionId, pRTP](const std::string msg) {
        std::cout << msg << std::endl;
        pRTP->DestroyStream(streamId);
        pRTP->DestroySession(sessionId);
        pRTP->Shutdown();
        return -1;
    };

    if (!streamId) {
        return tearDown("Failed to create a stream");

    }
    std::cout << "Generic Code finished" << std::endl;
/******** END OF GENERIC CODE *********/


/******** UVGRTP SPECIFIC CODE ********/
    auto pStream = UVGRTPWrap::GetSP(pRTP)->GetStream(streamId);
    if (!pStream) {
        return tearDown(std::string{"Failed to retrieve the "} + std::string{DIRECTION == 0 ? "out" : "in"} + std::string{" stream"});
    }

    // Set a receive hook for the stream
    if (DIRECTION  == 1) {
        if (pStream->install_receive_hook(nullptr, rtp_receive_hook) != RTP_OK) {
            return tearDown("Failed to install a receive hook");
        }
        std::cout << "Waiting for incoming packets.... " << std::endl;
        while(true);
    }
    else {

        /* Notice that PAYLOAD_MAXLEN > MTU (4096 > 1500).
         *
         * uvgRTP fragments all generic input frames that are larger than 1500 and in the receiving end,
         * it will reconstruct the full sent frame from fragments when all fragments have been received */

        auto media = std::unique_ptr<uint8_t[]>(new uint8_t[PAYLOAD_MAXLEN]);
        srand(time(NULL));

        for (int i = 0; i < TEST_PACKETS; ++i) {

            int random_packet_size = (rand() % PAYLOAD_MAXLEN) + 1;

            for (int i = 0; i < random_packet_size; ++i) {
                media[i] = (i + random_packet_size) % CHAR_MAX;
            }

            std::cout << "Sending RTP frame " << i + 1 << '/' << TEST_PACKETS << ". Payload size: "
                      << random_packet_size << std::endl;

            if (pStream->push_frame(media.get(), random_packet_size, RTP_NO_FLAGS) != RTP_OK) {
                return tearDown("Failed to send RTP frame");
            }
        }

    }

    // In a real implementation, you would receive this data from a remote endpoint using your chosen RTP library.
    // You can call the lambda handler here with the received data:
    // pRTP->OnRcvCallback(iStrmId, receivedData, receivedDataLength);

    // Destroy the streams and session when done (in a real implementation, handle errors and cleanup)
    pRTP->DestroyStream(streamId);
    pRTP->DestroySession(sessionId);

    // Shutdown the RTP wrapper
    pRTP->Shutdown();

    return 0;
}



