//
// Created by Julian Guarin on 10/11/23.
//

#ifndef AUDIOSTREAMPLUGIN_UTILITIES_H
#define AUDIOSTREAMPLUGIN_UTILITIES_H

#include <vector>
#include <utility>
#include <juce_audio_processors/juce_audio_processors.h>

class AudioStreamPluginProcessor;
namespace Utilities::Data
{
    void splitChannels (std::vector<std::vector<float>>& channels, const juce::AudioBuffer<float>& buffer);
    void joinChannels (juce::AudioBuffer<float>& buffer, const std::vector<std::vector<float>>& channels);
    void enumerateBuffer (juce::AudioBuffer<float>& buffer);
    void printAudioBuffer (const juce::AudioBuffer<float>& buffer);
    void printFloatBuffer (const std::vector<float>& buffer);

    /*!
     * @brief Interleaves a buffer into a vector of interleaved channels. Where each interleaved channel has 2 channels from buffer. If the number of channels is odd, the last channel is simply the last in buffer.
     * @param intChannels
     * @param buffer
     * @note intChannels MUST provide an adequate layout.
     */

    void interleaveBlocks (std::vector<std::vector<float>>& intChannels, juce::AudioBuffer<float>& buffer);
    void deinterleaveBlocks (std::vector<std::vector<float>>& blocks, std::vector<std::vector<float>>& interleavedBlocks);
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
#endif //AUDIOSTREAMPLUGIN_UTILITIES_H
