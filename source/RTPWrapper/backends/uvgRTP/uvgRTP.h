//
// Created by Julian Guarin on 6/10/23.
//

#ifndef RTPWRAPPER_UVGRTP_H
#define RTPWRAPPER_UVGRTP_H

#include "RTPWrap.h"
#include <uvgrtp/lib.hh>
#include "OpusWrapper.h"

//Type aliases
using SpSess = std::shared_ptr<uvgrtp::session>;
using SpStrm = std::shared_ptr<uvgrtp::media_stream>;
using WpSess = std::weak_ptr<uvgrtp::session>;
using WpStrm = std::weak_ptr<uvgrtp::media_stream>;

using StrmIndex     = std::map<uint64_t, SpStrm>;
using SessIndex     = std::map<uint64_t, SpSess>;
using SessStrmIndex = std::map<uint64_t, StrmIndex>;
using StrmSessIndex = std::map<uint64_t, uint64_t>;

class UVGRTPWrap : public RTPWrap {
   
 public:

    // UVG implementation for initialization
    uint64_t Initialize() override;

    // UVG implementation for creating a session
    uint64_t CreateSession(const std::string& localEndPoint) override;

    // UVG implementation for creating a stream
    uint64_t CreateStream(uint64_t sessionId, int srcPort, int destPort) override;

    // UVG implementation for destroying a stream
    bool DestroyStream(uint64_t streamId) override;

    // UVG implementation for destroying a session
    bool DestroySession(uint64_t sessionId) override;

    // UVG implementation for shutdown
    void Shutdown() override;

    SpSess GetSession(uint64_t sessionId);
    SpStrm GetStream(uint64_t streamId);

    static std::shared_ptr<UVGRTPWrap> GetSP(SPRTP p)
    {
        return std::dynamic_pointer_cast<UVGRTPWrap>(p);
    }

    ~UVGRTPWrap() override;

 private:
    uvgrtp::context ctx;

};

#endif //RTPWRAPPER_UVGRTP_H
