//
// Created by Julian Guarin on 23/01/24.
//
#include "RTPWrap.h"

namespace _rtpwrap
{
    std::recursive_mutex rmtx;
    static uint64_t _id = 0;
}

namespace _rtpwrap::data
{
    SpStrm GetStream(uint64_t sessionId, uint64_t streamId)
    {
        std::lock_guard<std::recursive_mutex> lock(rmtx);
        auto sessionFound = masterIndex.find(sessionId) != masterIndex.end();
        if (sessionFound && masterIndex[sessionId].find(streamId) != masterIndex[sessionId].end())
        {
            return masterIndex[sessionId][streamId];
        }
        return nullptr;
    }

    SpStrm GetStream(uint64_t streamId)
    {
        auto sessionId = 0ul;
        {
            std::lock_guard<std::recursive_mutex> lock(rmtx);
            if (streamIndex.find(streamId) != streamIndex.end()) {
                sessionId = streamIndex[streamId];
            }
        }
        return !sessionId ? nullptr : GetStream(sessionId, streamId);
    }

    SpSess GetSession(uint64_t sessionId)
    {
        std::lock_guard<std::recursive_mutex> lock(rmtx);
        if (sessionIndex.find(sessionId) != sessionIndex.end())
        {
            return sessionIndex[sessionId];
        }
        return nullptr;
    }

    uint64_t IndexStream(uint64_t sessionId, SpStrm stream)
    {
        if (stream == nullptr)
        {
            return 0;
        }
        std::lock_guard<std::recursive_mutex> lock(rmtx);
        auto streamId = ++_id;
        masterIndex[sessionId][streamId] = stream;
        streamIndex[streamId] = sessionId;
        return streamId;
    }

    uint64_t IndexSession(SpSess session)
    {
        if (session == nullptr)
        {
            return 0;
        }
        std::lock_guard<std::recursive_mutex> lock(rmtx);
        auto sessionId = ++_id;
        sessionIndex[sessionId] = session;
        return sessionId;
    }

    bool RemoveStream(uint64_t sessionId, uint64_t streamId)
    {
        std::lock_guard<std::recursive_mutex> lock(rmtx);
        auto sessionFound = masterIndex.find(sessionId) != masterIndex.end();
        if (sessionFound && masterIndex[sessionId].find(streamId) != masterIndex[sessionId].end())
        {
            masterIndex[sessionId].erase(streamId);
            streamIndex.erase(streamId);
            auto codecFound = streamIOCodec.find(streamId) != streamIOCodec.end();
            if (codecFound) streamIOCodec.erase(streamId);
            return true;
        }
        return false;
    }

    bool RemoveStream(uint64_t streamId)
    {
        auto sessionId = 0ul;
        {
            std::lock_guard<std::recursive_mutex> lock(rmtx);
            if (streamIndex.find(streamId) != streamIndex.end()) {
                sessionId = streamIndex[streamId];
            }
        }
        return RemoveStream(sessionId, streamId);
    }

    bool RemoveSession(uint64_t sessionId)
    {
        {
            std::lock_guard<std::recursive_mutex> lock(rmtx);
            auto sessionFound = masterIndex.find(sessionId) != masterIndex.end();
            if (!sessionFound) return false;
        }
        //Remove the streams
        auto strms = masterIndex[sessionId];
        for (auto& strm : strms)
        {
            RemoveStream(strm.first);
        }
        //Ok Kill the session
        {
            std::lock_guard<std::recursive_mutex> lock(rmtx);
            auto sessionFound = masterIndex.find(sessionId) != masterIndex.end();
            if (!sessionFound) return false;
            sessionFound = sessionIndex.find(sessionId) != sessionIndex.end();
            if (!sessionFound) return false;
            auto ptr = sessionIndex[sessionId];
            if (!ptr) return false;

            //TODO: The RTP Session for some reason cannot be hosted by a shared pointer. BUT I can tweak the uvgRTP code to manage it by myself. Now, the VST3 plugin fails to be copied when deleting the raw pointer. I need to change this cause it's big technical debt.
            //delete ptr;

            masterIndex.erase(sessionId);
            sessionIndex.erase(sessionId);
        }

        return true;
    }
}
