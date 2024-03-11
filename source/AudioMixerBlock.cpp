//
// Created by Julian Guarin on 17/11/23.
//

#include "AudioMixerBlock.h"

namespace Mixer
{
    Block SubBlocks(const Block&a, const Block&b)
    {
        auto topIndex = std::min(a.size(), b.size());
        std::vector<float> result(topIndex, 0.0f);
        for (auto index = 0ul; index < topIndex; ++index)
        {
            result[index] = a[index] - b[index];
        }

        return result;
    }

    Block AddBlocks(const Block&a, const Block&b)
    {
        auto topIndex = std::min(a.size(), b.size());
        std::vector<float> result;
        for (auto index = 0ul; index < topIndex; ++index)
            result.push_back(a[index] + b[index]);
        return result;
    }

    void AudioMixerBlock::addSource(TUserID sourceId)
    {
        std::lock_guard<std::recursive_mutex> lock(data_mutex);
        sourceIDToColumnIndex[sourceId] = sourceIDToColumnIndex.size();
        for(auto&kv:*this) kv.second.push_back(Block(mBlockSize, 0.0f));

        //TODO: Add the source to the RTP session IF the source is not local (sourceID != 0)
        /* This information comes from the session manager, here we need to define a queue to push the blocks
         * to the peer session thru RTP*/

    }

    void AudioMixerBlock::addColumn(int64_t timeIndex)
    {
        std::lock_guard<std::recursive_mutex> lock(data_mutex);
        playbackDataBlock[timeIndex] = Block(mBlockSize, 0.0f);
        this->operator[](timeIndex) = Column(sourceIDToColumnIndex.size(), Block(mBlockSize, 0.0f));
    }

    void AudioMixerBlock::layoutCheck(int64_t time, Mixer::TUserID sourceID)
    {
        //TIME Index doest not exist, add it
        if (this->find(time) == this->end())
        {
            addColumn(time);
        }
        //Source ID Indexing is not there (Add the Source).
        if (sourceIDToColumnIndex.find(sourceID) == sourceIDToColumnIndex.end())
        {
            addSource(sourceID);
        }
    }
    void AudioMixerBlock::replace(
        TTime time,
        const Block audioBlock,
        TUserID)
    {
        //SUPER SIMPLE
        {

            std::lock_guard<std::recursive_mutex> lock (data_mutex);
            size_t audioBlockSize = audioBlock.size();
            if (audioBlockSize != mBlockSize)
            {
                replacingBlockMismatch.Emit(audioBlockSize, mBlockSize);
                return;
            }
            playbackDataBlock[time] = audioBlock;
        }

    }

    void AudioMixerBlock::mix(
        int64_t time,
        const Block audioBlock,
        TUserID sourceID)
    {
        std::lock_guard<std::recursive_mutex> lock(data_mutex);
        if (mBlockSize != audioBlock.size())
        {
            return;
        }
        layoutCheck(time, sourceID);
        auto& column = this->operator[](time);
        auto& sourceIDIndex = sourceIDToColumnIndex[sourceID];
        auto& oldAudioBlock = column[sourceIDIndex];
        auto& oldPlayback = playbackDataBlock[time];

        //Update Local Audio Playback Header and source of Audio.
        playbackDataBlock[time] = SubBlocks(AddBlocks(oldPlayback, audioBlock), oldAudioBlock);
        oldAudioBlock = audioBlock;
    }

    void AudioMixerBlock::mix(
        std::vector<AudioMixerBlock>& mixers,
        int64_t time,
        const std::vector<Block>& splittedBlocks,
        Mixer::TUserID sourceID)
    {
        for (auto index = 0ul; index < mixers.size(); ++index)
        {
            mixers[index].mix(time, splittedBlocks[index], sourceID);
        }
    }

    void AudioMixerBlock::replace(
        std::vector<AudioMixerBlock>& mixers,
        int64_t time,
        const std::vector<Block>& splittedBlocks,
        Mixer::TUserID sourceID)
    {
        for (auto index = 0ul; index < mixers.size(); ++index)
        {
            mixers[index].replace(time, splittedBlocks[index], sourceID);
        }
    }

    Block AudioMixerBlock::getBlock(const int64_t time, int64_t& pbtime, bool delayed)
    {
        //Super Simple approach
        pbtime = !delayed ? time : time - static_cast<int64_t>(mDeltaBlocks * mBlockSize);

        std::lock_guard<std::recursive_mutex> lock(data_mutex);
        if (playbackDataBlock.find(pbtime) == playbackDataBlock.end())
        {
            //Add Silence.
            return Block(mBlockSize, 0.0f);
        }
        return playbackDataBlock[pbtime];
    }


    void AudioMixerBlock::flushMixer()
    {
        std::lock_guard<std::recursive_mutex> lock(data_mutex);
        playbackDataBlock.clear();
        sourceIDToColumnIndex.clear();
        static_cast<std::unordered_map<int64_t, Column>&>(*this).clear();
    }

    void AudioMixerBlock::resetMixer (size_t blockSize, uint32_t delayInSeconds)
    {
        std::lock_guard<std::recursive_mutex> lock(data_mutex);
        //Assuming 48k sample/second
        const auto delayInSamples = delayInSeconds * 48000;
        mDeltaBlocks = delayInSamples / blockSize;


        mBlockSize = blockSize;
        flushMixer();
    }

    void AudioMixerBlock::resetMixers(std::vector<AudioMixerBlock>& mixers, size_t blockSize, uint32_t delayInSeconds)
    {
        static std::mutex resetMutex;
        std::lock_guard<std::mutex> lock(resetMutex);
        for (auto& mixer : mixers)
        {
            mixer.resetMixer(blockSize, delayInSeconds);
        }
    }

}