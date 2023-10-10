//
// Created by Julian Guarin on 6/10/23.
//

#ifndef RTPWRAPPER_MOCKRTP_H
#define RTPWRAPPER_MOCKRTP_H

#include "RTPWrap.h"

class MockRTPWrap : public RTPWrap {

 public:

    // Mock implementation for initialization
    uint64_t Initialize() override;

    // Mock implementation for creating a session
    uint64_t CreateSession(const std::string& endpoint) override;

    // Mock implementation for creating a stream
    uint64_t CreateStream(int srcPort, int destPort) override;

    // Mock implementation for setting a receive callback
    // You can implement your logic here to handle the callback
    void SetOnRcvCallback(uint64_t streamId, RcvHandler callback = [](const uint64_t) -> void {}) override;

    // Mock implementation for setting a transmit callback
    // You can implement your logic here to handle the callback
    void SetOnTrxCallback(uint64_t streamId, TrxHandler callback = [](const uint64_t) -> void {}) override;

    // Mock implementation for setting an error callback
    // You can implement your logic here to handle the callback
    void SetOnErrCallback(uint64_t streamId, ErrHandler callback = [](const uint64_t) -> void {}) override;

    // Mock implementation for destroying a stream
    bool DestroyStream(uint64_t streamId) override;

    // Mock implementation for destroying a session
    bool DestroySession(uint64_t sessionId) override;

    // Mock implementation for shutdown
    void Shutdown() override;

    ~MockRTPWrap() override;
};

#endif //RTPWRAPPER_MOCKRTP_H
