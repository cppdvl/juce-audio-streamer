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
    uint64_t CreateSession(const std::string& localEndPoint) override;

    // Mock implementation for creating a stream
    uint64_t CreateStream(uint64_t sessionId, int srcPort, int destPort) override;

    // Mock implementation for destroying a stream
    bool DestroyStream(uint64_t streamId) override;

    // Mock implementation for destroying a session
    bool DestroySession(uint64_t sessionId) override;

    // Mock implementation for shutdown
    void Shutdown() override;

    ~MockRTPWrap() override;
};

#endif //RTPWRAPPER_MOCKRTP_H
