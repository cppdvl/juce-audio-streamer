#include "uvgRTP.h"

#include <iostream>

// TCP PORT 
constexpr uint16_t REMOTE_PORT = 8888;

// demonstration parameters of the example
constexpr uint32_t PAYLOAD_MAXLEN = (0xffff - 0x1000);
constexpr int TEST_PACKETS = 100;




void rtp_receive_hook(void* arg, uvgrtp::frame::rtp_frame* pframe)
{
    std::cout << "Received a frame" << std::endl;
    uvgrtp::frame::dealloc_frame(pframe);

}


int main()
{

    //THIS WAS CREATED WITH SOLELY PURPOSE OF COMPILE CHECK THE UVGRTPWrapper made sense. 
    //Of course we need to use the general Interface.
    //UVGRTPWrapper.
    



/****************        FACTORY      ****************/
    SPRTP pRTP = std::make_shared<UVGRTPWrap>();
    
    std::cout << "Wrapper Instanced" << std::endl;

/********************* GENERIC CODE TO CREATE A SESSION AND A STREAM **************************/
    // Initialize the RTP wrapper
    uint64_t wrapperHandle = pRTP->Initialize();

    // Create a session
    std::cout << "Creating a session" << std::endl;
    uint64_t sessionId = pRTP->CreateSession("localhost");
    
    if (!sessionId) {
        std::cout << "Failed to create a session" << std::endl;
        return -1;
    }
    

    // Create two streams for sending and receiving
    std::cout << "Creating a stream" << std::endl;
    uint64_t oStrmId = pRTP->CreateStream(sessionId, REMOTE_PORT, 0);  //0: 0utput
    uint64_t iStrmId = pRTP->CreateStream(sessionId, REMOTE_PORT, 1);  //1: 1nput
    
    auto tearDown = [oStrmId, iStrmId, sessionId, pRTP](const std::string msg) {
        std::cout << msg << std::endl;
        pRTP->DestroyStream(oStrmId);
        pRTP->DestroyStream(iStrmId);
        pRTP->DestroySession(sessionId);
        pRTP->Shutdown();
        return -1;
    };

    if (!oStrmId || !iStrmId) {
        return tearDown("Failed to create a stream");
        
    }
    std::cout << "Generic Code finished" << std::endl;
/******** END OF GENERIC CODE *********/


/******** UVGRTP SPECIFIC CODE ********/
    auto pIStrm = UVGRTPWrap::GetSP(pRTP)->GetStream(iStrmId);
    auto pOStrm = UVGRTPWrap::GetSP(pRTP)->GetStream(oStrmId);
    if (!pIStrm || !pOStrm) {
        return tearDown(std::string{"Failed to retrieve the "} + std::string{(!pIStrm ? "in" : "out")} + std::string{" stream"});
    }
    
    // Set a receive hook for the stream
    if (pIStrm->install_receive_hook(nullptr, rtp_receive_hook) != RTP_OK)
    {
        return tearDown("Failed to install a receive hook");
    }

    std::cout << "Waiting for incoming packets.... " << std::endl; 


    /* Notice that PAYLOAD_MAXLEN > MTU (4096 > 1500).
     *
     * uvgRTP fragments all generic input frames that are larger than 1500 and in the receiving end,
     * it will reconstruct the full sent frame from fragments when all fragments have been received */

    auto media = std::unique_ptr<uint8_t[]>(new uint8_t[PAYLOAD_MAXLEN]);
    srand(time(NULL));

    for (int i = 0; i < TEST_PACKETS; ++i){

        int random_packet_size = (rand() % PAYLOAD_MAXLEN) + 1;
        
        for (int i = 0; i < random_packet_size; ++i){
            media[i] = (i + random_packet_size) % CHAR_MAX;
        }

        std::cout << "Sending RTP frame " << i + 1 << '/' << TEST_PACKETS << ". Payload size: " << random_packet_size << std::endl;

        if (pOStrm->push_frame(media.get(), random_packet_size, RTP_NO_FLAGS) != RTP_OK) {
            return tearDown("Failed to send RTP frame");
        }
    }

    // Simulate sending data on the sendStream
    std::string sendData = "Hello, this is a test!";
    

    // In a real implementation, you would send this data to a remote endpoint using your chosen RTP library.
    
    // Simulate receiving data on the receiveStream
    const char* receivedData = "This is a response.";
    size_t receivedDataLength = strlen(receivedData);
    
    // In a real implementation, you would receive this data from a remote endpoint using your chosen RTP library.
    // You can call the lambda handler here with the received data:
    // pRTP->OnRcvCallback(iStrmId, receivedData, receivedDataLength);

    // Destroy the streams and session when done (in a real implementation, handle errors and cleanup)
    pRTP->DestroyStream(oStrmId);
    pRTP->DestroyStream(iStrmId);
    pRTP->DestroySession(sessionId);

    // Shutdown the RTP wrapper
    pRTP->Shutdown();

    return 0;
}


