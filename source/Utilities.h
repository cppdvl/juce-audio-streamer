//
// Created by Julian Guarin on 10/11/23.
//

#ifndef AUDIOSTREAMPLUGIN_UTILITIES_H
#define AUDIOSTREAMPLUGIN_UTILITIES_H

#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>

namespace Utilities::Data
{
    void splitChannels(std::vector<std::vector<float>>& channels, const juce::AudioBuffer<float>& buffer);
    void joinChannels(juce::AudioBuffer<float>& buffer, const std::vector<std::vector<float>>& channels);
    void enumerateBuffer(juce::AudioBuffer<float>& buffer);
    void printAudioBuffer(const juce::AudioBuffer<float>& buffer);
    void printFloatBuffer(const std::vector<float>& buffer);
    void interleaveChannels (std::vector<std::vector<float>>& intChannels, juce::AudioBuffer<float>& buffer);
    void deinterleaveChannels (juce::AudioBuffer<float>& deintBuffer, std::vector<std::vector<float>>& channels);
    void deinterleaveChannels (juce::AudioBuffer<float>& deintBuffer, std::vector<float>& buffer);
}

#endif //AUDIOSTREAMPLUGIN_UTILITIES_H
