//
// Created by Julian Guarin on 10/11/23.
//

#include "Utilities.h"

namespace Utilities::Data
{
    std::tuple <bool, uint32_t, int64_t, std::vector<std::byte>> extractIncomingData (std::vector<std::byte>& uid_ts_encodedPayload)
    {
        //[ UID [0-3] | TS [4-7] | PAYLOAD [8-N] ]
        auto&src = uid_ts_encodedPayload;
        if ( src.size() < 8) return std::make_tuple(false, 0, 0, ByteBuff {});
        auto extractLast = [](auto&source, auto const n)-> ByteBuff {return ByteBuff(std::make_move_iterator(source.begin()+n), std::make_move_iterator(source.end()));};
        ByteBuff payLoad(extractLast(src, 8));
        int64_t nSample(*reinterpret_cast<uint32_t*>(extractLast(src, 4).data()));
        uint32_t userID(*reinterpret_cast<uint32_t*>(src.data()));
        return std::make_tuple(true, userID, nSample, payLoad);
    }

    void splitChannels (std::vector<std::vector<float>>& channels, const juce::AudioBuffer<float>& buffer, const bool monoSplit)
    {
        auto numSamp = static_cast<size_t> (buffer.getNumSamples());
        auto numChan = static_cast<size_t> (buffer.getNumChannels());
        channels.resize (numChan, std::vector<float> (numSamp, 0.0f));
        auto rdPtrs = buffer.getArrayOfReadPointers();
        for (auto channelIndex = 0lu; channelIndex < numChan; ++channelIndex)
        {
            std::copy (rdPtrs[channelIndex], rdPtrs[channelIndex] + numSamp, channels[channelIndex].begin());
            if (monoSplit && (channelIndex % 2))
            {
                for (auto sampleIndex = 0lu; sampleIndex < numSamp; ++sampleIndex)
                {
                    channels[channelIndex][sampleIndex] += channels[channelIndex-1][sampleIndex];
                    channels[channelIndex][sampleIndex] *= 0.5f;
                    channels[channelIndex-1][sampleIndex] = channels[channelIndex][sampleIndex];
                }
            }
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

    std::vector<float> interleaveBlocks(std::vector<float>& block0, std::vector<float>& block1)
    {
        jassert(block0.size() == block1.size());
        std::vector<float> intBlock{};
        auto numSamples = static_cast<size_t>(block0.size());
        for(auto sampleIndex = 0lu; sampleIndex < numSamples; ++sampleIndex)
        {
            intBlock.push_back(block0[sampleIndex]);
            intBlock.push_back(block1[sampleIndex]);
        }
        return intBlock;
    }
    void interleaveBlocks(std::vector<std::vector<float>>& intBlocks, std::vector<std::vector<float>>&blocks)
    {
        jassert(blocks.size() % 2 == 0);
        intBlocks.clear();
        for (auto index = 0lu; index < blocks.size(); index += 2)
        {
            intBlocks.push_back(interleaveBlocks(blocks[index], blocks[index+1]));
        }
    }

    void interleaveBlocks (std::vector<std::vector<float>>& intChannels, juce::AudioBuffer<float>& buffer)
    {
        auto& is = intChannels;
        auto& b = buffer;
        //layout
        auto numChan = static_cast<size_t> (b.getNumChannels());
        //Force Even Channels
        bool forcingEven = numChan & 1;
        auto numSamp = static_cast<size_t> (b.getNumSamples());
        is.resize ((numChan + (forcingEven ? 1 : 0)) >> 1, std::vector<float> (numSamp << 1, 0.0f));
        //interleave and force even parity
        auto _rdPtrs = b.getArrayOfReadPointers(); auto rdPtrs = std::vector<const float*>(_rdPtrs, _rdPtrs + numChan);
        std::vector<float> lstChann = std::vector<float> (numSamp, 0.0f);
        if (forcingEven) rdPtrs.push_back(lstChann.data());
        jassert(rdPtrs.size() % 2 == 0);
        jassert(forcingEven ^ (rdPtrs[numChan - 1] != lstChann.data()));


        //FROM THIS POINT ON ASSUME EVEN PARITY FOR THE NUMBER OF CHANNELS
        //interleave
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

    }

    std::vector<std::vector<float>> deinterleaveBlock(std::vector<float>& interleavedBlocks);
    std::vector<std::vector<float>> deinterleaveBlock(std::vector<float>& interleavedBlocks)
    {
        jassert(interleavedBlocks.size() % 2 == 0);
        auto bothBlocksSize = interleavedBlocks.size() >> 1;
        auto blocks = std::vector<std::vector<float>>(2, std::vector<float>(bothBlocksSize, 0.0f));
        for (auto index = 0lu; index < interleavedBlocks.size(); ++index)
        {
            blocks[index % 2][index >> 1] = interleavedBlocks[index];
        }
        return blocks;
    }

    void deinterleaveBlocks (std::vector<std::vector<float>>&dBlocks,std::vector<float>&iBlocks)
    {
        dBlocks.clear();
        dBlocks.push_back(std::vector<float>{});
        dBlocks.push_back(std::vector<float>{});
        for (auto index = 0lu; index < iBlocks.size(); ++index)
        {
            dBlocks[index % 2].push_back(iBlocks[index]);
        }
    }

    void deinterleaveBlocks (std::vector<std::vector<float>>& blocks, std::vector<std::vector<float>>& interleavedBlocks)
    {
        blocks = std::vector<std::vector<float>>(interleavedBlocks.size() * 2, std::vector<float>(interleavedBlocks[0].size() >> 1, 0.0f));
        for (auto index = 0lu; index < interleavedBlocks.size(); index+=2)
        {
            auto deinterleavedBlocks = deinterleaveBlock(interleavedBlocks[index]);
            blocks[index] = deinterleavedBlocks[0];
            blocks[index + 1] = deinterleavedBlocks[1];
        }
    }

}
namespace Utilities::Time
{
    std::tuple<uint32_t, int64_t> getPosInMSAndSamples (juce::AudioPlayHead* playHead)
    {
        if (!playHead) return std::make_tuple(0, NoPlayHead);

        auto positionInfo = playHead->getPosition();

        if (!positionInfo.hasValue()) return std::make_tuple(0, -1);

        auto optionalTimeSamples = positionInfo->getTimeInSamples();
        if (!optionalTimeSamples.hasValue()) return std::make_tuple(0, NoTimeSamples);

        auto optionalTimeSeconds = positionInfo->getTimeInSeconds();
        if (!optionalTimeSeconds.hasValue()) return std::make_tuple(0, NoTimeMS);

        auto timeSeconds = *optionalTimeSeconds;
        auto timeSamples = *optionalTimeSamples;
        return std::make_tuple(static_cast<uint32_t>(timeSeconds * 1000), timeSamples);
    }
}
