//
// Created by Julian Guarin on 10/11/23.
//

#include "Utilities.h"

namespace Utilities::Data
{

    void splitChannels (std::vector<std::vector<float>>& channels, const juce::AudioBuffer<float>& buffer)
    {
        auto numSamp = static_cast<size_t> (buffer.getNumSamples());
        auto numChan = static_cast<size_t> (buffer.getNumChannels());
        channels.resize (numChan, std::vector<float> (numSamp, 0.0f));
        auto rdPtrs = buffer.getArrayOfReadPointers();
        for (auto channelIndex = 0lu; channelIndex < numChan; ++channelIndex)
        {
            std::copy (rdPtrs[channelIndex], rdPtrs[channelIndex] + numSamp, channels[channelIndex].begin());
        }
    }

    void joinChannels (juce::AudioBuffer<float>& buffer, const std::vector<std::vector<float>>& channels)
    {
        buffer.clear();
        auto wrPtrs = buffer.getArrayOfWritePointers();
        auto numChan = static_cast<size_t> (buffer.getNumChannels());
        for (auto channelIndex = 0lu; channelIndex < numChan; ++channelIndex)
        {
            std::copy (channels[channelIndex].begin(), channels[channelIndex].end(), wrPtrs[channelIndex]);
        }
        buffer.setNotClear();
    }

    void enumerateBuffer (juce::AudioBuffer<float>& buffer)
    {
        auto numSamp = static_cast<size_t> (buffer.getNumSamples());
        auto numChan = static_cast<size_t> (buffer.getNumChannels());
        auto totalSamp = numChan * numSamp;
        std::vector<float> f (totalSamp, 0.0f);
        std::iota (f.begin(), f.end(), 0);
        for (auto idx_ : f)
        {
            auto idx = static_cast<size_t> (idx_);
            buffer.getWritePointer (static_cast<int> (idx / numSamp))[idx % numSamp] = idx_;
        }
    }
    void printAudioBuffer (const juce::AudioBuffer<float>& buffer)
    {
        auto totalSamp_ = static_cast<size_t> (buffer.getNumSamples() * buffer.getNumChannels());
        auto numSamp = static_cast<size_t> (buffer.getNumSamples());
        auto rdPtr = buffer.getReadPointer (0);
        for (size_t index = 0; index < totalSamp_; ++index)
        {
            std::cout << "[" << index / numSamp << ", " << index % numSamp << "] = " << rdPtr[index] << " -- ";
        }
        std::cout << std::endl;
    }
    void printFloatBuffer (const std::vector<float>& buffer)
    {
        auto totalSamp = buffer.size();
        for (size_t index = 0; index < totalSamp; ++index)
        {
            std::cout << "[" << index << "] = " << buffer[index] << " -- ";
        }
        std::cout << std::endl;
    }

    /*!
     * @brief Interleaves a buffer into a vector of interleaved channels. Where each interleaved channel has 2 channels from buffer. If the number of channels is odd, the last channel is simply the last in buffer.
     * @param intChannels
     * @param buffer
     */
    void interleaveChannels (std::vector<std::vector<float>>& intChannels, juce::AudioBuffer<float>& buffer)
    {
        auto& is = intChannels;
        auto& b = buffer;

        //layout
        auto numChan = static_cast<size_t> (b.getNumChannels());
        auto numSamp = static_cast<size_t> (b.getNumSamples());
        is.resize (numChan >> 1, std::vector<float> (numSamp << 1, 0.0f));

        //interleave
        auto rdPtrs = b.getArrayOfReadPointers();
        auto channelIndexOffset = 0lu;
        for (auto& i : is)
        {
            std::iota (i.begin(), i.end(), 0.0f);
            std::transform (i.begin(), i.end(), i.begin(), [rdPtrs, channelIndexOffset] (float& idx_) -> float {
                auto idx = static_cast<size_t> (idx_);
                auto channelIndex = channelIndexOffset + (idx % 2);
                auto rdOffset = idx / 2;
                return rdPtrs[channelIndex][rdOffset];
            });
            channelIndexOffset += 2;
        }

        //last channel (if odd)
        if (numChan % 2)
        {
            std::vector<float> lstChann = std::vector<float> (numSamp, 0.0f);
            std::copy (rdPtrs[numChan - 1], rdPtrs[numChan - 1] + numSamp, lstChann.begin());
            is.push_back (lstChann);
        }
    }
    void interleaveChannels (std::vector<float>& intBuffer, juce::AudioBuffer<float>& buffer);
    void interleaveChannels (std::vector<float>& intBuffer, juce::AudioBuffer<float>& buffer)
    {
        intBuffer.clear();

        auto& i = intBuffer;
        auto& b = buffer;
        auto numSamp = static_cast<size_t> (b.getNumSamples());
        auto numChan = static_cast<size_t> (b.getNumChannels());
        numChan = numChan > 2 ? 2 : numChan;
        auto readPointersPerChannel = buffer.getArrayOfReadPointers();

        if (numChan == 1)
        {
            std::copy (b.getReadPointer (0), b.getReadPointer (0) + numSamp, std::back_inserter (i));
            return;
        }
        //more than 1 channel
        i.resize ((size_t) numSamp * (size_t) numChan, 0.0f);
        std::iota (i.begin(), i.end(), 0);
        std::transform (i.begin(), i.end(), i.begin(), [readPointersPerChannel, numChan] (float& idx_) -> float {
            auto idx = static_cast<size_t> (idx_);
            auto channelIndex = idx % numChan;
            auto rdOffset = idx / numChan;
            return readPointersPerChannel[channelIndex][rdOffset];
        });
        b.setNotClear();
    }
    void deinterleaveChannels (juce::AudioBuffer<float>& deintBuffer, std::vector<std::vector<float>>& channels)
    {
        deintBuffer.clear();
        auto& d = deintBuffer;
        auto& cs = channels;
        auto wrPtrs = d.getArrayOfWritePointers();
        auto numChan = static_cast<size_t> (d.getNumChannels());
        auto numSamp = static_cast<size_t> (d.getNumSamples());

        for (auto cIdx = 0lu; cIdx < numChan; ++cIdx)
        {
            auto& c = cs[cIdx / 2];
            auto totalChannels = c.size() / numSamp;
            for (auto sIdx = 0lu; sIdx < numSamp; ++sIdx)
            {
                wrPtrs[cIdx][sIdx] = c[sIdx * totalChannels + cIdx % 2];
            }
        }
        deintBuffer.setNotClear();
    }
    void deinterleaveChannels (juce::AudioBuffer<float>& deintBuffer, std::vector<float>& buffer)
    {
        deintBuffer.clear();
        auto& d = deintBuffer;
        auto& b = buffer;
        auto numSamp = static_cast<size_t> (d.getNumSamples());
        auto numChan = static_cast<size_t> (d.getNumChannels());
        numChan = numChan > 2 ? 2 : numChan;
        auto totalSamp = numChan * numSamp;
        jassert (totalSamp == b.size() && numChan > 0);

        auto writePointersPerChannel = d.getArrayOfWritePointers();
        for (auto sampleIndex = 0lu; sampleIndex < totalSamp; ++sampleIndex)
        {
            writePointersPerChannel[sampleIndex % numChan][sampleIndex / numChan] = b[sampleIndex];
        }
        deintBuffer.setNotClear();
    }

}
