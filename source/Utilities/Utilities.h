//
// Created by Julian Guarin on 10/11/23.
//

#ifndef AUDIOSTREAMPLUGIN_UTILITIES_H
#define AUDIOSTREAMPLUGIN_UTILITIES_H

#include "RTPWrap.h"
#include "Utilities/Buffer/BlockSizeAdapter.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class AudioStreamPluginProcessor;
namespace Utilities::Buffer
{
    enum class OpResult
    {
        Success,
        InvalidOperands,
        UnkownError
    };
    using ByteBuff = std::vector<std::byte>;
    void splitChannels (std::vector<Buffer::BlockSizeAdapter>& bsa, const juce::AudioBuffer<float>& buffer, const bool monoSplit = false);
    void splitChannels (std::vector<std::vector<float>>& channels, const juce::AudioBuffer<float>& buffer, const bool monoSplit = false);
    void joinChannels (juce::AudioBuffer<float>& buffer, const std::vector<std::vector<float>>& channels);
    //void joinChannels (juce::AudioBuffer<float>& buffer, const std::vector<Buffer::BlockSizeAdapter>& bsa);
    void enumerateBuffer (juce::AudioBuffer<float>& buffer);
    void printAudioBuffer (const juce::AudioBuffer<float>& buffer);
    void printFloatBuffer (const std::vector<float>& buffer);

    /*!
     * @brief Interleaves a buffer into a vector of interleaved channels. Where each interleaved channel has 2 channels from buffer. If the number of channels is odd, the last channel is simply the last in buffer.
     * @param intChannels
     * @param buffer
     * @note intChannels MUST provide an adequate layout.
     */

    std::vector<float> interleaveBlocks(std::vector<float>& block0, std::vector<float>& block1);
    void interleaveBlocks (std::vector<std::vector<float>>& intChannels, juce::AudioBuffer<float>& buffer);
    void interleaveBlocks (std::vector<std::vector<float>>& interBlocks, std::vector<std::vector<float>>& blocks);
    //void interleaveBlocks (std::vector<std::vector<float>>& interBlocks, std::vector<Buffer::BlockSizeAdapter>& blocks);


    void deinterleaveBlocks (std::vector<std::vector<float>>& blocks, std::vector<std::vector<float>>& interleavedBlocks);
    void deinterleaveBlocks (std::vector<std::vector<float>>&,std::vector<float>&);

    std::tuple<bool, uint32_t, int64_t, std::vector<std::byte>> extractIncomingData(std::vector<std::byte>& uid_ts_encodedPayload);

    OpResult monoSplit (std::vector<float>& left, std::vector<float>& right);
}

namespace Utilities::Time{

    constexpr int64_t NoPlayHead = -1;
    constexpr int64_t NoPositionInfo = -2;
    constexpr int64_t NoTimeSamples = -3;
    constexpr int64_t NoTimeMS = -4;

    /*!
     * @brief Get the position indexes in milliseconds and sample position.
     * @return
     */

    std::tuple<uint32_t, int64_t> getPosInMSAndSamples (juce::AudioPlayHead*);
}


namespace Utilities::Network{
    //Local Interfaces
    std::map<std::string, std::map<std::string, std::string>> getNetworkInfo();
    std::vector<std::string> getNetworkInterfaces();

    //RTP Sessions
    void createSession(SPRTP spRTP,
        uint64_t& sessionID,
        uint64_t& streamOutID,
        uint64_t& streamInID,
        int& outPort,
        int& inPort,
        std::string ip);
    void createSession();
    uint64_t createOutStream(SPRTP spRTP,
        uint64_t sessionID,
        uint64_t& streamOutID,
        int& outPort);
    void createSession(AudioStreamPluginProcessor&);
    uint64_t createInStream(SPRTP spRTP,
        uint64_t sessionID,
        uint64_t& streamInID,
        int& outPort,
        int& inPort);

}

#endif //AUDIOSTREAMPLUGIN_UTILITIES_H
