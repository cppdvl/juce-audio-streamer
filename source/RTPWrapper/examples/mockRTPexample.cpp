#include "mockRTP.h"


int main()
{
    //THIS WAS CREATED WITH SOLELY PURPOSE OF COMPILE CHECK THE MockRTPWrapper made sense. Of course we need to use the general Interface.
    //MockRTPWrapper.
    
    
    MockRTPWrap mockWrapper;

    // Initialize the RTP wrapper
    uint64_t wrapperHandle = mockWrapper.Initialize();

    // Create a session
    uint64_t sessionHandle = mockWrapper.CreateSession("localhost");

    // Create two streams for trxing and receiving
    uint64_t trxStreamId = mockWrapper.CreateStream(1234, 5678);
    uint64_t rcvStreamId = mockWrapper.CreateStream(5678, 1234);

    // Set a lambda handler for the receiving stream
    mockWrapper.SetOnRcvCallback(rcvStreamId);

    // Simulate trxing data on the sendStream
    std::string trxData = "Hello, this is a test!";
    // In a real implementation, you would trx this data to a remote endpoint using your chosen RTP library.
    
    // Simulate receiving data on the rcvStream
    const char* rcvdData = "This is a response.";
    size_t rcvdDataLength = strlen(receivedData);
    
    // In a real implementation, you would rcv this data from a remote endpoint using your chosen RTP library.
    // You can call the lambda handler here with the rcvd data:
    // mockWrapper.OnRcvCallback(rcvStreamId, receivedData, receivedDataLength);

    // Destroy the streams and session when done (in a real implementation, handle errors and cleanup)
    mockWrapper.DestroyStream(trxStreamId);
    mockWrapper.DestroyStream(rcvStreamId);
    mockWrapper.DestroySession(sessionHandle);

    // Shutdown the RTP wrapper
    mockWrapper.Shutdown();

    return 0;
}


