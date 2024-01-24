//
// Created by Julian Guarin on 22/01/24.
//

#ifndef AUDIOSTREAMPLUGIN_UDPRTP_H
#define AUDIOSTREAMPLUGIN_UDPRTP_H
#include "RTPWrap.h"

#define UDPRTP_MAXSIZE 512

#include "xlet.h"
using sessiontoken  = uint64_t;
using streamtoken   = xlet::UDPInOut;





class UDPRTPWrap : public RTPWrap
{
    std::function<void(const uint64_t, std::vector<std::byte>&)> __receivingHook{[](auto,auto&)->void{}};

public:

    uint64_t Initialize() override;
    /**
     * @brief Create a session and return a session handle.
     * @param The remote endpoint to "connect" to.
     * @return uint64_t A handle for the created session.
     */
    uint64_t CreateSession(const std::string& remoteIp) override;
    uint64_t CreateStream(uint64_t sessionId, int remotePort, int direction = 2) override;
    bool PushFrame(std::vector<std::byte> pData, uint64_t streamId, uint32_t timestamp) override;
    bool DestroyStream(uint64_t streamId) override;
    bool DestroySession(uint64_t sessionId) override;
    void Shutdown() override;


private:

    /*! \brief The peer id in the networtk (THIS IS NOT A DAW AudioStream User ID)*/
    uint64_t __peerId{0};
    uint32_t __uid{0};
};
#endif //AUDIOSTREAMPLUGIN_UDPRTP_H
