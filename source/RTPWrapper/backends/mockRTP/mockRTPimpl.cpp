#include "mockRTP.h"

#include <iostream>

uint64_t MockRTPWrap::Initialize()   {
    //Initialize mock
    return 0;
}

uint64_t MockRTPWrap::CreateSession(const std::string& endpoint)   {
    // Mock implementation for creating a session
    return 2; // Return a placeholder handle
}

uint64_t MockRTPWrap::CreateStream(int srcPort, int destPort)   {
    // Mock implementation for creating a stream
    return 3; // Return a placeholder handle
}

void MockRTPWrap::SetOnRcvCallback(uint64_t streamId, RcvHandler callback)   {
        // Mock implementation for setting a receive callback
        // You can implement your logic here to handle the callback
        mRcvHandler = callback;
    }

void MockRTPWrap::SetOnTrxCallback(uint64_t streamId, TrxHandler callback)   {
        // Mock implementation for setting a transmit callback
        // You can implement your logic here to handle the callback
        mTrxHandler = callback;
    }

void MockRTPWrap::SetOnErrCallback(uint64_t streamId, TrxHandler callback)   {
        // Mock implementation for setting an error callback
        // You can implement your logic here to handle the callback 
        mErrHandler = callback;
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


