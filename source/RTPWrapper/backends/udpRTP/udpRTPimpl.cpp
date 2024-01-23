//
// Created by Julian Guarin on 22/01/24.

// The following implementation needs a relay router on the cloud for this to work.
// We really need to implement a TURN service.

//
#include "udpRTP.h"


uint64_t UDPRTPWrap::Initialize()
{
    if (__udpDeafInOut)
    {
        __udpDeafInOut->run();
    }
    return 0;
}

uint64_t UDPRTPWrap::CreateSession(const std::string& remoteEndPointIp)
{
    __remoteEndPointIP = remoteEndPointIp;
    return 0;
}
uint64_t UDPRTPWrap::CreateStream(uint64_t, int srcPort, int)
{
    __remoteEndPointPort = srcPort;

    //IP, Port, Do not bind, uses a Q to go send data.
    __udpDeafInOut = new xlet::UDPInOut (__remoteEndPointIP, __remoteEndPointPort, false, true);

    auto __userId_sockaddr_in = xlet::UDPlet::toSystemSockAddr(__remoteEndPointIP, __remoteEndPointPort);
    __peerId = xlet::UDPlet::sockAddrIpToUInt64(__userId_sockaddr_in);

    if (__peerId == 0)
    {
        __userId = 0;
        return 0;
    }

    __userId = static_cast<uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    return __userId;
}

bool UDPRTPWrap::DestroyStream(uint64_t)
{
    if (__udpDeafInOut)
    {
        delete __udpDeafInOut;
        __udpDeafInOut = nullptr;
    }
    __userId = 0;
    __peerId = 0;
    return true;
}

void UDPRTPWrap::Shutdown()
{
    DestroyStream(0);
}

bool UDPRTPWrap::PushFrame(std::vector<std::byte> data, uint32_t ts)
{

    std::vector<std::vector<std::byte>> ret{std::vector<std::byte>{}};
    std::vector<std::byte>& tmp = ret.back();
    for (auto&b : data)
    {
        tmp.push_back(b);
        if (tmp.size() == (UDPRTP_MAXSIZE+1))
        {
            ret.push_back(std::vector<std::byte>{});
            tmp = ret.back();
        }
    }
    return PushFrames(ret, ts);
}

bool UDPRTPWrap::PushFrame(uint64_t, const std::vector<std::byte>) noexcept
{
    return true;
}
bool UDPRTPWrap::PushFrame(uint64_t, const std::vector<std::byte> pData, uint32_t ts) noexcept
{
    //So reliable MTU size is 576 bytes. We need to split the data in chunks of 576 bytes.
    // [FRAMES | FRAMEINDEX | DATA]
    std::vector<std::byte> frame2transmit{};
    auto pts = reinterpret_cast<std::byte*>(&ts);
    frame2transmit.insert(frame2transmit.begin(), pts, pts+4);
    frame2transmit.insert(frame2transmit.end(), pData.begin(), pData.end());
    if (__udpDeafInOut)
    {
        //Push it for transmission.
        //Insert frame2transmit at the beginning of pData.
        __udpDeafInOut->qout_.push(std::make_pair(__peerId, frame2transmit));
        //frame2transmit [TS_0 | TS_1 | TS_2 | TS_3 | FRAMES | FRAME_INDEX | UID | DATA ]
        return true;
    }
    return false;
}

bool UDPRTPWrap::PushFrames(std::vector<std::vector<std::byte>> frames, uint32_t ts)
{
    unsigned char frameIndex = 0;
    auto framesz = static_cast<std::byte>(frames.size() & 0xFF);
    auto bUID = static_cast<std::byte>(__userId & 0xFF);
    for (auto&frame : frames)
    {
        frame.insert(frame.begin(), bUID);
        frame.insert(frame.begin(), std::byte(frameIndex++));
        frame.insert(frame.begin(), std::byte(framesz));
        PushFrame(0, frame, ts);
    }
    return true;
}
void UDPRTPWrap::SetReceivingHook(std::function<void(const uint64_t, std::vector<std::byte>&)> hook)
{
    if (__udpDeafInOut)
    {
        __receivingHook = hook;
    }
}

