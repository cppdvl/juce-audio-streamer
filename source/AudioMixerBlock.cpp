//
// Created by Julian Guarin on 17/11/23.
//

#include <numeric>
#include <iostream>
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
        std::cout << "About to add Blocks" << std::endl;
        std::vector<float> result;
        for (auto index = 0ul; index < BlockSize; ++index)
            result.push_back(a[index] + b[index]);
        std::cout << "About to finish Blocks" << std::endl;
        return result;
    }

    void AudioMixerBlock::addSource(int32_t sourceId)
    {
        sourceIDToColumnIndex[sourceId] = sourceIDToColumnIndex.size();
        for(auto&kv:*this) kv.second.push_back(Block(BlockSize, 0.0f));
    }

    void AudioMixerBlock::addColumn(int64_t timeIndex)
    {
        playbackData[timeIndex] = Block(BlockSize, 0.0f);
        this->operator[](timeIndex) = Column(sourceIDToColumnIndex.size(), Block(BlockSize, 0.0f));
    }
    void AudioMixerBlock::layoutCheck(int64_t time, int32_t sourceID)
    {
        //TIME Index doest not exist, add it
        if (this->find(time) == this->end()) addColumn(time);
        //Source ID Indexing is not there (Add the Source).
        if (sourceIDToColumnIndex.find(sourceID) == sourceIDToColumnIndex.end()) addSource(sourceID);
    }

    void AudioMixerBlock::mix(int64_t time, const Block block, std::unordered_map<int32_t, std::vector<Block>>& blocksToStream, int32_t sourceID)
    {
        std::lock_guard<std::recursive_mutex> lock(data_mutex);
        if (playbackData.find(time) == playbackData.end())
        {
            std::cout << "New Kid on The Block: " << time << std::endl;
        }
        layoutCheck(time, sourceID);
        auto& column = this->operator[](time);
        auto columnSize = column.size();

        auto& sourceIDIndex = sourceIDToColumnIndex[sourceID];
        auto& oldblock = column[sourceIDIndex];
        auto& oldPlayback = playbackData[time];

        //If only the track playhead is present the column size is 1 and nothing needs to be streamed
        if (columnSize < 1)
        {
            playbackData[time] = block;
            return;
        }

        auto newPlaybackData = SubBlocks(AddBlocks(oldPlayback, block), oldblock);

        for(auto& sourceID_columnIndex : sourceIDToColumnIndex)
        {
            auto& columnIndex = sourceID_columnIndex.second;

            //We are not going to stream to ourselves (!columnIndex)
            //We are not going to stream the data in-streamed (columnIndex == sourceIDIndex)
            if (!columnIndex || columnIndex == sourceIDIndex) continue;

            auto& blockAtIndex = column[columnIndex];
            blocksToStream[sourceID_columnIndex.first].push_back(SubBlocks(newPlaybackData, blockAtIndex));
        }

        playbackData[time] = newPlaybackData;
    }

    Block AudioMixerBlock::getBlock(int64_t time)
    {
        std::lock_guard<std::recursive_mutex> lock(data_mutex);
        return playbackData[time];
    }

    Column AudioMixerBlock::getStreamColumn(int64_t timeIndex)
    {

        //guard.
        std::lock_guard<std::recursive_mutex> lock(data_mutex);
        if (this->find(timeIndex) == this->end()) addColumn(timeIndex);


        //Get Data
        auto& audioBlock = *this;
        Column& column = audioBlock[timeIndex];
        auto& mixedBlock = playbackData[timeIndex];

        //Create result streamColumn
        Column  strcol = Column(column.size(), Block(BlockSize, 0.0f));

        //operate
        std::vector<size_t> range(strcol.size(), 0); std::iota(range.begin(), range.end(), 0);

        for (auto& i : range)
        {
            auto& block  = column[i];
            auto& result = strcol[i];
            for (size_t sampleIndex = 0; sampleIndex < BlockSize; ++sampleIndex) result[sampleIndex] = mixedBlock[sampleIndex] - block[sampleIndex];
        }

        //return result.
        return strcol;

    }




}