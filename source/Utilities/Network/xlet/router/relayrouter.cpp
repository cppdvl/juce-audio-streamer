//
// Created by Julian Guarin on 16/01/24.
//

#include "xlet.h"

static std::set<uint64_t>   authorizedPeers{};
static std::set<uint64_t>   peers{};
static uint64_t             hostingPeerId{0};
static bool                 requiredAuth{true};
DAWn::Events::Signal<uint64_t>  nonAuthorizedPeerAttemptedAConnection;
static std::set<uint64_t>   attemptedPeers{};


//TODO:UGLY HACK Change this.
using RTABLE = std::array<std::set<uint64_t>, 2>;

static std::map<uint64_t, std::shared_ptr<xlet::UDPInOut>> studios_router{};
static std::map<uint64_t, RTABLE> studios_routing{};
static std::map<uint64_t, std::map<uint32_t, uint64_t>> studios_uidtopeerid{};
static std::map<uint64_t, std::set<uint64_t>> studios_authorizedPeers{};

std::shared_ptr<xlet::UDPInOut> CreateRouter(std::string, int, bool, bool);
uint64_t CreateStudio(std::string ip, int port)
{
    auto sckadd             = xlet::UDPlet::toSystemSockAddr(ip,port);
    uint64_t studioIndex    = xlet::UDPlet::sockAddToPeerId(sckadd);

    auto studioFound = studios_router.find(studioIndex) != studios_router.end();
    if (studioFound)
    {
        auto sp = studios_router[studioIndex];
        if (sp)
        {
            if (sp->valid()) return studioIndex;
        }
        studios_router.erase(studioIndex);
        return 0;
    }

    auto spRouter = CreateRouter(ip, port, true, true);
    if (!spRouter)
    {
        return 0;
    }

    studios_router[studioIndex] = spRouter;
    studios_routing[studioIndex] = RTABLE{};
    studios_uidtopeerid[studioIndex] = std::map<uint32_t, uint64_t>{};
    studios_authorizedPeers[studioIndex] = std::set<uint64_t>{};

    return studioIndex;
}

bool DestroyStudio(uint64_t studioIndex)
{
    auto authorizedPeersFound = studios_authorizedPeers.find(studioIndex) != studios_authorizedPeers.end();
    if (authorizedPeersFound)
    {
        studios_authorizedPeers.erase(studioIndex);
    }

    auto routingFound = studios_routing.find(studioIndex) != studios_routing.end();
    if (!routingFound)
    {
        studios_routing.erase(studioIndex);
    }

    auto uid2peerFound = studios_uidtopeerid.find(studioIndex) != studios_uidtopeerid.end();
    if (!uid2peerFound)
    {
        studios_uidtopeerid.erase(studioIndex);
    }

    auto studioFound = studios_router.find(studioIndex) != studios_router.end();
    if (studioFound)
    {
        auto sp = studios_router[studioIndex];

        if (sp)
        {
            if (sp->valid())
            {
                close(sp->getSocket());
                studios_router.erase(studioIndex);
                return true;
            }

        }
    }
    return false;
}

bool UpdateStudio(uint64_t studioIndex, std::vector<uint64_t> peers)
{

    if (studios_router.find(studioIndex) == studios_router.end()) {
        std::cout << "Studio Not Found: Std.Index " << studioIndex << std::endl; return false;
    }

    auto spRouter = studios_router[studioIndex];
    if (!spRouter)
    {
        std::cout << "Weak Ptr Expired: Std.Index " << studioIndex << std::endl; DestroyStudio(studioIndex); return false;
    }
    else if(!spRouter->valid())
    {
        std::cout << "Invalid Socket: Std.Index " << studioIndex << std::endl; DestroyStudio(studioIndex); return false;
    }

    auto studioFound = studios_authorizedPeers.find(studioIndex) != studios_authorizedPeers.end();
    if (studioFound)
    {
        studios_authorizedPeers.erase(studioIndex);
    }

    studios_authorizedPeers[studioIndex] = std::set<uint64_t>(peers.begin(), peers.end());
    return true;

}

bool isAuthorizedPeer(uint64_t studioIndex, uint64_t peerId)
{
    auto studioFound = studios_authorizedPeers.find(studioIndex) != studios_authorizedPeers.end();
    if (!studioFound) return false;
    auto& authPeers = studios_authorizedPeers[studioIndex];
    return (!requiredAuth) || (authPeers.find(peerId) != authPeers.end());
}

std::shared_ptr<xlet::UDPInOut> CreateRouter (std::string ip, int port, bool listen, bool synced)
{
    auto serverIdSckAdd = xlet::UDPlet::toSystemSockAddr(ip, port);
    auto studioId = xlet::UDPlet::sockAddToPeerId(serverIdSckAdd);
    auto routerFound = studios_router.find(studioId) != studios_router.end();
    std::shared_ptr<xlet::UDPInOut> spRouter;
    if (routerFound)
    {
        spRouter = studios_router[studioId];
        if (spRouter)
        {
            std::cout << "Router already exists: " << xlet::UDPlet::letIdToString(studioId) << std::endl;
            if (spRouter->valid()) return spRouter;
            std::cout << "But it was an invalid router. Creating a new one." << std::endl;
        }
        studios_router.erase(studioId);
        if (studios_routing.find(studioId) != studios_routing.end()) studios_routing.erase(studioId);
        if (studios_uidtopeerid.find(studioId) != studios_uidtopeerid.end()) studios_uidtopeerid.erase(studioId);
        if (studios_authorizedPeers.find(studioId) != studios_authorizedPeers.end()) studios_authorizedPeers.erase(studioId);
    }

    spRouter = std::make_shared<xlet::UDPInOut>(ip, port, listen, synced);



    spRouter->letOperationalError.Connect(std::function<void(uint64_t, std::string)>{[](uint64_t peerId, std::string error) {
        std::cout << peerId << " " << error << std::endl;
    }});
    spRouter->letBindedOn.Connect(std::function<void(uint64_t, std::thread::id)>{[](uint64_t peerId, std::thread::id threadId) {
        std::cout << peerId << " " << threadId << std::endl;
    }});
    spRouter->letThreadStarted.Connect(std::function<void(uint64_t)>{[](uint64_t peerId) {
        std::cout << peerId << " Thread Started" << std::endl;
    }});

    spRouter->letDataFromPeerIsReady.Connect(
    std::function<void(uint64_t, std::vector<std::byte>&)>{
        [studioId](uint64_t peerId, std::vector<std::byte>& data){
            auto studioFound = studios_router.find(studioId) != studios_router.end();
            if (!studioFound)
            {
                std::cout << "CRITICAL ERROR: Studio not found: " << xlet::UDPlet::letIdToString(studioId) << std::endl;
                return;
            }
            auto ptrRouter = studios_router[studioId];
            if (!ptrRouter)
            {
                std::cout << "CRITICAL ERROR: Studio not found: " << xlet::UDPlet::letIdToString(studioId) << std::endl;
                return;
            }

            if (!isAuthorizedPeer(studioId, peerId)) return;

            auto routingAvailable = studios_routing.find(studioId) != studios_routing.end();
            auto studioAvailable = studios_router.find(studioId) != studios_router.end();
            auto uid2peerAvailable = studios_uidtopeerid.find(studioId) != studios_uidtopeerid.end();


            //Check studio is indexed
            if (!routingAvailable || !studioAvailable || !uid2peerAvailable)
            {
                std::cout << "CRITICAL ERROR: Routing (" << routingAvailable << ") or Studio (" << studioAvailable << ") or uid2peer(" << uid2peerAvailable <<") not available" << std::endl;
                return;
            }

            auto& routing = studios_routing[studioId];
            auto& uid2peer = studios_uidtopeerid[studioId];
            uint32_t uid = *(reinterpret_cast<uint32_t*>(data.data()));

            //Update Routing Table: Mixer Node (Multicaster is index 0) and Studio Node (Multicaster is index 1)
            auto isMulticastUID     = uid%2 == 0;
            auto isUnicastUID       = !isMulticastUID;
            size_t multicasterIndex = 0;
            size_t unicasterIndex   = 1;


            auto& routeFromSet      = routing[isMulticastUID ? multicasterIndex : unicasterIndex];
            auto& routeToSet        = routing[isMulticastUID ? unicasterIndex : multicasterIndex];

            auto notFound = routeFromSet.find(uid) == routeFromSet.end();

            if (notFound)
            {
                if (isMulticastUID){

                    //This is group 0, eliminate all uids
                    for (auto&routeFromUID : routeFromSet)
                        uid2peer.erase(routeFromUID);

                    routeFromSet.clear();

                }
                routeFromSet.insert(uid);
            }

            //Associate UID with Peer Network ID
            if (uid2peer.find(uid) == uid2peer.end())
            {
                uid2peer[uid] = peerId;
            }

            //ENROUTE
            for (auto& routeToUID : routeToSet)
            {
                auto& toPeerID = uid2peer[routeToUID];
                ptrRouter->qout_.push(std::make_pair(toPeerID, data));
            }

        }
    });
    return spRouter;
}


void disableAuthRequirement()
{
    requiredAuth = false;
}

void enableAuthRequirement()
{
    requiredAuth = true;
}

void addAuthorizedPeer(uint64_t peerId)
{
    authorizedPeers.insert(peerId);
}

void removeAuthorizedPeer(uint64_t peerId)
{
    authorizedPeers.erase(peerId);
}

std::shared_ptr<xlet::UDPInOut> GetRouter(uint64_t studioId)
{
    auto studioFound = studios_router.find(studioId) != studios_router.end();
    if (!studioFound)
    {
        std::cout << "CRITICAL ERROR: Studio not found: " << xlet::UDPlet::letIdToString(studioId) << std::endl;
        return nullptr;
    }
    auto ptrRouter = studios_router[studioId];
    if (!ptrRouter)
    {
        std::cout << "CRITICAL ERROR: Studio not found: " << xlet::UDPlet::letIdToString(studioId) << std::endl;
        return nullptr;
    }
    if (ptrRouter->valid())
    {
        return ptrRouter;
    }
    else
    {
        std::cout << "CRITICAL ERROR: Studio not valid: " << xlet::UDPlet::letIdToString(studioId) << std::endl;
        return nullptr;
    }
}





