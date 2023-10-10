#include "uvgRTP.h"

#include <iostream>

uint64_t UVGRTPWrap::Initialize()   {
    //Initialize mock
    return 0;
}

uint64_t UVGRTPWrap::CreateSession(const std::string& endpoint)   {
    // UVG implementation for creating a session
    return 2; // Return a placeholder handle
}

uint64_t UVGRTPWrap::CreateStream(int srcPort, int destPort)   {
    // UVG implementation for creating a stream
    return 3; // Return a placeholder handle
}

void UVGRTPWrap::SetOnRcvCallback(uint64_t streamId, RcvHandler callback)   {
        // UVG implementation for setting a receive callback
        // You can implement your logic here to handle the callback
        mRcvHandler = callback;
    }

void UVGRTPWrap::SetOnTrxCallback(uint64_t streamId, TrxHandler callback)   {
        // UVG implementation for setting a transmit callback
        // You can implement your logic here to handle the callback
        mTrxHandler = callback;
    }

void UVGRTPWrap::SetOnErrCallback(uint64_t streamId, TrxHandler callback)   {
        // UVG implementation for setting an error callback
        // You can implement your logic here to handle the callback 
        mErrHandler = callback;
    }

bool UVGRTPWrap::DestroyStream(uint64_t streamId)   {
        // UVG implementation for destroying a stream
        return true; // Return a placeholder result
    }

bool UVGRTPWrap::DestroySession(uint64_t sessionId)   {
        // UVG implementation for destroying a session
        return true; // Return a placeholder result
    }

void UVGRTPWrap::Shutdown()   {
        // UVG implementation for shutdown
    }

UVGRTPWrap::~UVGRTPWrap()   {
        // UVG implementation for destruction
    }


