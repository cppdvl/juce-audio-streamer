#include "uvgRTP.h"

#include <memory> 


void rcvCb(uint64_t id)
{
}


using SP  = std::shared_ptr<RTPWrap>;
int main()
{
    //THIS WAS CREATED WITH SOLELY PURPOSE OF COMPILE CHECK THE UVGRTPWrapper made sense. Of course we need to use the general Interface.
    //UVGRTPWrapper.
    
    
    SP pRTP = std::make_shared<UVGRTPWrap>();


    // Initialize the RTP wrapper
    uint64_t wrapperHandle = pRTP->Initialize();

    // Create a session
    uint64_t sessionHandle = pRTP->CreateSession("localhost");

    // Create two streams for sending and receiving
    uint64_t sendStreamHandle = pRTP->CreateStream(1234, 5678);
    uint64_t receiveStreamHandle = pRTP->CreateStream(5678, 1234);

    // Set a lambda handler for the receiving stream
    pRTP->SetOnRcvCallback(receiveStreamHandle, rcvCb);

    // Simulate sending data on the sendStream
    std::string sendData = "Hello, this is a test!";
    // In a real implementation, you would send this data to a remote endpoint using your chosen RTP library.
    
    // Simulate receiving data on the receiveStream
    const char* receivedData = "This is a response.";
    size_t receivedDataLength = strlen(receivedData);
    
    // In a real implementation, you would receive this data from a remote endpoint using your chosen RTP library.
    // You can call the lambda handler here with the received data:
    // pRTP->OnRcvCallback(receiveStreamHandle, receivedData, receivedDataLength);

    // Destroy the streams and session when done (in a real implementation, handle errors and cleanup)
    pRTP->DestroyStream(sendStreamHandle);
    pRTP->DestroyStream(receiveStreamHandle);
    pRTP->DestroySession(sessionHandle);

    // Shutdown the RTP wrapper
    pRTP->Shutdown();

    return 0;
}


