#include "mockRTP.h"

#include <iostream>

uint64_t MockRTPWrap::Initialize()   {
    //Initialize mock
    return 0;
}

uint64_t MockRTPWrap::CreateSession(const std::string& localEndPoint){
    // Mock implementation for creating a session
    return 2; // Return a placeholder handle
}

uint64_t MockRTPWrap::CreateStream(uint64_t sessionId, int srcPort, int destPort)   {
    // Mock implementation for creating a stream
    return 3; // Return a placeholder handle
}

bool MockRTPWrap::DestroyStream(uint64_t streamId)   {
        // Mock implementation for destroying a stream
        return true; // Return a placeholder result
    }

bool MockRTPWrap::DestroySession(uint64_t sessionId)   {
        // Mock implementation for destroying a session
        return true; // Return a placeholder result
    }

void MockRTPWrap::Shutdown()   {
        // Mock implementation for shutdown
    }

MockRTPWrap::~MockRTPWrap()   {
        // Mock implementation for destruction
    }


