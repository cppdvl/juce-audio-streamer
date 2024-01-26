//
// Created by Julian Guarin on 22/01/24.

// The following implementation needs a relay router on the cloud for this to work.
// We really need to implement a TURN service.

//
#include "RTPWrap.h"


static uint32_t generateUniqueID() {
    static std::mt19937 generator(std::random_device{}()); // Initialize once with a random seed
    std::uniform_int_distribution<uint32_t> distribution;
    return distribution(generator);
}

uint64_t UDPRTPWrap::Initialize()
{
    return 0;
}

uint64_t UDPRTPWrap::CreateSession(const std::string& remoteEndPointIp)
{
    auto sckaddr    = xlet::UDPlet::toSystemSockAddr(remoteEndPointIp, 0);
    __peerId        = xlet::UDPlet::sockAddrIpToUInt64(sckaddr);
    return          _rtpwrap::data::IndexSession(&__peerId);
}
uint64_t UDPRTPWrap::CreateStream(uint64_t sessionId, int remotePort, int)
{
    //recalc peerId
    std::string remoteIp    = xlet::UDPlet::letIdToIpString(__peerId);
    auto sckaddr            = xlet::UDPlet::toSystemSockAddr(remoteIp, remotePort);

    __uid                   = generateUniqueID();
    __peerId                = xlet::UDPlet::sockAddrIpToUInt64(sckaddr);

    //IP, Port, Do not bind or listen, is qsynced to send and receive data.
    auto streamID           = _rtpwrap::data::IndexStream(sessionId, std::shared_ptr<xlet::UDPInOut>(new xlet::UDPInOut (remoteIp, remotePort, false, true)));
    return streamID;
}

bool UDPRTPWrap::DestroyStream(uint64_t streamId)
{
    return _rtpwrap::data::RemoveStream(streamId);
}
bool UDPRTPWrap::DestroySession(uint64_t sessionId)
{
    return _rtpwrap::data::RemoveSession(sessionId);
}

void UDPRTPWrap::Shutdown()
{
    DestroyStream(0);
}

bool UDPRTPWrap::PushFrame(std::vector<std::byte> pData, uint64_t streamId, uint32_t timestamp)
{
    auto pStrm = _rtpwrap::data::GetStream(streamId);
    if (!pStrm) return false;

    auto pts = reinterpret_cast<std::byte*>(&timestamp);
    auto puid = reinterpret_cast<std::byte*>(&__uid);

    pData.insert(pData.begin(), pts, pts+4);    //[TS | DATA]
    pData.insert(pData.begin(), puid, puid+4);  //[UID | TS | DATA]

    pStrm->qout_.push(std::make_pair(__peerId, pData));

    return true;
}
