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
    void interleaveChannels (std::vector<std::vector<float>>& intChannels, juce::AudioBuffer<float>& buffer);
    void deinterleaveChannels (juce::AudioBuffer<float>& deintBuffer, std::vector<std::vector<float>>& channels);
    void deinterleaveChannels (juce::AudioBuffer<float>& deintBuffer, std::vector<float>& buffer);
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
