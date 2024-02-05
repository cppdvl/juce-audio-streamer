//
// Created by Julian Guarin on 17/11/23.
//

#include "AudioMixerBlock.h"

namespace Mixer
{
    Block SubBlocks(const Block&a, const Block&b)
    {
        std::vector<float> result;
        for (auto index = 0ul; index < BlockSize; ++index)
            result.push_back(a[index] - b[index]);
        return result;
    }

    Block AddBlocks(const Block&a, const Block&b)
    {
        std::vector<float> result;
        for (auto index = 0ul; index < BlockSize; ++index)
            result.push_back(a[index] + b[index]);
        return result;
    }

    void AudioMixerBlock::addSource(TUserID sourceId)
    {
        sourceIDToColumnIndex[sourceId] = sourceIDToColumnIndex.size();
        for(auto&kv:*this) kv.second.push_back(Block(BlockSize, 0.0f));

        //TODO: Add the source to the RTP session IF the source is not local (sourceID != 0)
        /* This information comes from the session manager, here we need to define a queue to push the blocks
         * to the peer session thru RTP*/

    }

    void AudioMixerBlock::addColumn(int64_t timeIndex)
    {
        playbackDataBlock[timeIndex] = Block(BlockSize, 0.0f);
        this->operator[](timeIndex) = Column(sourceIDToColumnIndex.size(), Block(BlockSize, 0.0f));
    }

    void AudioMixerBlock::layoutCheck(int64_t time, Mixer::TUserID sourceID)
    {
        //TIME Index doest not exist, add it
        if (this->find(time) == this->end()) addColumn(time);
        //Source ID Indexing is not there (Add the Source).
        if (sourceIDToColumnIndex.find(sourceID) == sourceIDToColumnIndex.end()) addSource(sourceID);
    }

    void AudioMixerBlock::mix(
        int64_t time,
        const Block audioBlock,
        TUserID sourceID,
        std::unordered_map<TUserID, std::vector<Block>>&)
    {
        std::lock_guard<std::recursive_mutex> lock(data_mutex);
        layoutCheck(time, sourceID);
        auto& column = this->operator[](time);
        auto& sourceIDIndex = sourceIDToColumnIndex[sourceID];
        auto& oldAudioBlock = column[sourceIDIndex];
        auto& oldPlayback = playbackDataBlock[time];

        //Update Local Audio Playback Header and source of Audio.
        playbackDataBlock[time] = SubBlocks(AddBlocks(oldPlayback, audioBlock), oldAudioBlock);
        oldAudioBlock = audioBlock;

        /*for(auto& sourceID_columnIndex : sourceIDToColumnIndex)
        {
            auto& columnIndex = sourceID_columnIndex.second;
            auto& userID = sourceID_columnIndex.first;

            //We are not going to stream to ourselves (userID == 0)
            //We are not going to stream the data the source data streamd in (userID == sourceID)
            if (!userID || userID == sourceID) continue;

            //Remember, audioBlock to update is the new information for the userID
            auto& blockToUpdate = column[columnIndex];
            blockToUpdate = SubBlocks(playbackDataBlock[time], blockToUpdate);
            blocksToStream[userID].push_back(blockToUpdate);
        }*/
    }

    void AudioMixerBlock::mix(
        std::vector<AudioMixerBlock>& mixers,
        int64_t time,
        const std::vector<Block>& splittedBlocks,
        Mixer::TUserID sourceID)
    {

        std::unordered_map<TUserID, std::vector<Block>> blocksToStream{};
        std::vector<Block> playbackHead {};

        for (auto index = 0ul; index < mixers.size(); ++index)
        {
            mixers[index].mix(time, splittedBlocks[index], sourceID, blocksToStream);
            playbackHead.push_back(mixers[index].getBlock(time));
        }
        AudioMixerBlock::mixFinished.Emit(playbackHead, time);
    }

    Block AudioMixerBlock::getBlock(int64_t time)
    {
        std::lock_guard<std::recursive_mutex> lock(data_mutex);
        return playbackDataBlock[time];
    }

    const Row& AudioMixerBlock::getPlaybackDataBlock()
    {
        return playbackDataBlock;
    }

    bool AudioMixerBlock::hasData (int64_t time)
    {
        std::lock_guard<std::recursive_mutex> lock(data_mutex);
        return this->find(time) != this->end();
    }
    bool AudioMixerBlock::invalid (std::vector<AudioMixerBlock>& mixers, int64_t time)
    {
        //if only one channel doesn't have data, imply there's no data at all.
        auto isInvalid = std::any_of (mixers.begin(), mixers.end(), [time] (AudioMixerBlock& mixer) -> bool {
            return mixer.hasNotData(time);
        });

        return isInvalid;
    }

}