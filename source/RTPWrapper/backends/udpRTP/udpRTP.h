//
// Created by Julian Guarin on 22/01/24.
//

#ifndef AUDIOSTREAMPLUGIN_UDPRTP_H
#define AUDIOSTREAMPLUGIN_UDPRTP_H
#include "RTPWrap.h"

#define UDPRTP_MAXSIZE 512

#include <xlet.h>
class UDPRTPWrap : public RTPWrap
{
    xlet::UDPInOut* __udpDeafInOut {nullptr};
    uint64_t        __peerId {0};
    std::string     __remoteEndPointIP;
    int             __remoteEndPointPort{0};
    uint64_t        __userId {0};
    std::function<void(const uint64_t, std::vector<std::byte>&)> __receivingHook{[](auto,auto&)->void{}};
public:

    uint64_t Initialize() override;
    uint64_t CreateSession(const std::string& remoteIp) override;
    uint64_t CreateStream(uint64_t sessionId, int remotePort, int direction = 0) override;
    inline uint64_t CreateStream(int remotePort) { return CreateStream(0, remotePort); }
    bool DestroyStream(uint64_t streamId) override;
    void Shutdown() override;

    /*! \brief This push frame in not listening UDPInOut is not implemented for US. */
    bool PushFrame(uint64_t, const std::vector<std::byte>) noexcept override;

    /*! \brief Push a frame of data.
     *
     * @param streamId useless as there's only one "connection".
     * @param pData Data to transport.
     * @param ts TimeStamp.
     * @return true if the frame was successfully pushed, false otherwise.
     */
    bool PushFrame(uint64_t streamId, const std::vector<std::byte> pData, uint32_t ts) noexcept override;
    void SetReceivingHook(std::function<
        void(const uint64_t, std::vector<std::byte>&)
        >);
    bool PushFrame(std::vector<std::byte> data, uint32_t ts);
    bool PushFrames(std::vector<std::vector<std::byte>> frames, uint32_t ts);

};
#endif //AUDIOSTREAMPLUGIN_UDPRTP_H
