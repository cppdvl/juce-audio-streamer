//
// Created by Julian Guarin on 26/12/23.
//

#ifndef WSCONNECT_STREAMMANAGER_H
#define WSCONNECT_STREAMMANAGER_H
#include "Events.h"

#include <string>
#include <vector>
#include <cstddef>

namespace DAWn {

    struct StreamTransport
    {
        std::string mIp{};
        uint16_t    mPort{0};
    };
    struct StreamConfiguration
    {
        uint16_t    mPort{0};
        int32_t     mSampleRate{48000};
        int         mBlockSize{480};
        int         mChannels{2};
        int         mDirection{2}; // 0: input, 1: output, 2: both
        bool        mUseOpus{true};
    };
    class StreamManager {
        StreamConfiguration mConfiguration;
        uint64_t mSessionId{0};

    public:
        StreamManager() = default;
        uint64_t Initialize();
        uint64_t CreateStream(std::string ip, uint16_t port);
        uint64_t DestroyStream(uint64_t sessionId, uint64_t streamId);
        bool     PushFrame(uint64_t sessionId, std::vector<std::byte>pData, uint32_t timeStamp = 0);

        DAWn::Events::Signal<uint64_t> OnStreamCreated;
        DAWn::Events::Signal<uint64_t> OnStreamDestroyed;
        DAWn::Events::Signal<uint64_t, std::vector<std::byte>> OnFramePushed;
        DAWn::Events::Signal<uint64_t, std::vector<std::byte>> OnFrameGrabbed;

    };
}

#endif //WSCONNECT_STREAMMANAGER_H
