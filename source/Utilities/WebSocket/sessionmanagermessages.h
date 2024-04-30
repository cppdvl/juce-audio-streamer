//
// Created by Julian Guarin on 18/12/23.
//

#ifndef WSCONNECT_SESSIONMANAGERMESSAGES_H
#define WSCONNECT_SESSIONMANAGERMESSAGES_H
#include "nlohmann/json.hpp"
using json = nlohmann::json;
namespace DAWn::Messages
{
    static json AudioSettingsChanged(int32_t sampleRate, int32_t blockSize, int bitDepth)
    {
        const json kPayLoad
        {
        {"SampleRate", sampleRate},
        {"BlockSize",  blockSize},
        {"BitDepth",   bitDepth}
        };

        const json kData
        {
        {"Type",      5},
        {"Payload",   kPayLoad},
        {"TimeStamp", std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count())}
        };

        const json kMessage
        {
        {"action",  "pluginMessage"},
        {"data",    kData}
        };

        return kMessage;

    }

    static json KeepAlive()
    {
        const json kData
        {
        {"Type",      7},
        {"Payload",   nullptr},
        {"TimeStamp", std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count())}
        };
        const json kKeepAlive
        {
        {"action",  "pluginMessage"},
        {"data",    kData}
        };

        return kKeepAlive;
    }

    static json Disconnect()
    {
        const json kDisconnect {
        {"Type",    8},
        {"Payload", nullptr}
        };

        return kDisconnect;
    }

    static json PlaybackCommand(uint8_t command, uint32_t timePosition)
    {
        const json kPayload
        {
          {"Command", command},
          {"TimePosition", timePosition}
        };

        const json kCommand
        {
          {"Type", 13},
          {"Payload", kPayload},
          {"TimeStamp", std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())}
        };

        const json kWrapper
        {
          {"action", "pluginMessage"},
          {"data", kCommand}
        };

        return kWrapper;
    }
}

#endif //WSCONNECT_SESSIONMANAGERMESSAGES_H
