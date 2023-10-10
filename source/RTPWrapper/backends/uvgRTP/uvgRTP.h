//
// Created by Julian Guarin on 6/10/23.
//

#ifndef RTPWRAPPER_UVGRTP_H
#define RTPWRAPPER_UVGRTP_H

#include "RTPWrap.h"
#include <uvgrtp/lib.hh>



class UVGRTPWrap : public RTPWrap {
   
 public:

    // UVG implementation for initialization
    uint64_t Initialize() override;

    // UVG implementation for creating a session
    uint64_t CreateSession(const std::string& endpoint) override;

    // UVG implementation for creating a stream
    uint64_t CreateStream(int srcPort, int destPort) override;

    // UVG implementation for setting a receive callback
    // You can implement your logic here to handle the callback
    void SetOnRcvCallback(uint64_t streamId, RcvHandler callback = [](const uint64_t) -> void {}) override;

    // UVG implementation for setting a transmit callback
    // You can implement your logic here to handle the callback
    void SetOnTrxCallback(uint64_t streamId, TrxHandler callback = [](const uint64_t) -> void {}) override;

    // UVG implementation for setting an error callback
    // You can implement your logic here to handle the callback
    void SetOnErrCallback(uint64_t streamId, ErrHandler callback = [](const uint64_t) -> void {}) override;

    // UVG implementation for destroying a stream
    bool DestroyStream(uint64_t streamId) override;

    // UVG implementation for destroying a session
    bool DestroySession(uint64_t sessionId) override;

    // UVG implementation for shutdown
    void Shutdown() override;

    ~UVGRTPWrap() override;

 private:
    uvgrtp::context ctx;

};

#endif //RTPWRAPPER_UVGRTP_H
