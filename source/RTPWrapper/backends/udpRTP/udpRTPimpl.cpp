//
// Created by Julian Guarin on 22/01/24.

// The following implementation needs a relay router on the cloud for this to work.
// We really need to implement a TURN service.

//
#include "RTPWrap.h"

#include <unordered_map>

struct DataCache : std::unordered_map<uint32_t, std::vector<std::byte>>
{

    bool IsCached(uint32_t ts)
    {
        auto it = this->find(ts);
        if (it == this->end()) return false;
        return true;
    }

    std::vector<std::byte>& GetCached(uint32_t ts)
    {
        return this->at(ts);
    }

    void Cache(uint32_t ts, std::vector<std::byte> data)
    {
        this->operator[](ts) = data;
    }

};
static DataCache __dataCache{};
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

//TODO: In the future this will need to be refactored. We should have both userId and direction.
uint64_t UDPRTPWrap::CreateStream(uint64_t sessionId, int remotePort, int userId = 0)
{
    //recalc peerId
    uint32_t ui32userId     = static_cast<uint32_t>(userId);
    std::string remoteIp    = xlet::UDPlet::letIdToIpString(__peerId);
    auto sckaddr            = xlet::UDPlet::toSystemSockAddr(remoteIp, remotePort);

    __uid                   = ui32userId != 0 ? ui32userId : generateUniqueID();
    __peerId                = xlet::UDPlet::sockAddToPeerId(sckaddr);

    //IP, Port, Do not bind or listen, is qsynced to send and receive data.
    auto streamID           = _rtpwrap::data::IndexStream(sessionId, std::shared_ptr<xlet::UDPInOut>(new xlet::UDPInOut (remoteIp, remotePort, false, true)));
    return streamID;
}
uint64_t UDPRTPWrap::CreateLoopBackStream(uint64_t sessionId, int remotePort, int userId = 0)
{
    auto ui32userId = static_cast<uint32_t>(userId);
    __uid = userId != 0 ? ui32userId : generateUniqueID();
    auto streamID = _rtpwrap::data::IndexStream(sessionId, std::shared_ptr<xlet::UDPInOut>(new xlet::UDPInOut ("127.0.0.1", remotePort, false, true, true)));
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

    if (pData.size() == 0)
    {
        std::cout << "NO DATA !!!!!!!!!!!!!!!!!!!!!" << std::endl;
    }
    pStrm->qout_.push_back(xlet::Data{.first = __peerId, .second = pData});

    return true;
}
bool UDPRTPWrap::__dataIsCached (uint64_t streamId, uint32_t timestamp)
{
    if(!__dataCache.IsCached(timestamp))
        return false;
    auto pStrm = _rtpwrap::data::GetStream(streamId);
    //Grab Cached Buffer and send
    auto pData = __dataCache.GetCached(timestamp);
    pStrm->qout_.push_back(xlet::Data{.first = __peerId, .second = pData});
    return true;

}
void UDPRTPWrap::__cacheData (uint32_t timestamp, std::vector<std::byte>& pData)
{
    auto pts = reinterpret_cast<std::byte*>(&timestamp);
    auto puid = reinterpret_cast<std::byte*>(&__uid);
    pData.insert(pData.begin(), pts, pts+4);    //[TS | DATA]
    pData.insert(pData.begin(), puid, puid+4);  //[UID | TS | DATA]
    __dataCache.Cache(timestamp, pData);
}

void UDPRTPWrap::__clearCache()
{
    __dataCache.clear();
}