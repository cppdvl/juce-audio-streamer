//
// Created by Julian Guarin on 17/11/23.
//

#include <numeric>
#include "AudioMixerBlock.h"

namespace Mixer
{
    void AudioMixerBlock::addSource(int32_t sourceId, int64_t timeIndex, const Block& block)
    {
        sourceIDToColumnIndex[sourceId] = sourceIDToColumnIndex.size();
        for(auto&kv:*this) kv.second.push_back(timeIndex != -1 && kv.first == timeIndex ? block : Block(BlockSize, 0.0f));
    }

    void AudioMixerBlock::addColumn(int64_t timeIndex)
    {
        this->operator[](timeIndex) = Column(sourceIDToColumnIndex.size(), Block(BlockSize, 0.0f));
    }

    void AudioMixerBlock::mix(int64_t time, const Block& block, int32_t sourceID)
    {
        std::lock_guard<std::recursive_mutex> lock(data_mutex);

        //Lazy push
        if (this->find(time) == this->end()) addColumn(time);
        if (sourceIDToColumnIndex.find(sourceID) == sourceIDToColumnIndex.end()) addSource(sourceID, time, block);
        else this->operator[](time)[sourceIDToColumnIndex[sourceID]] = block;

        auto sourceIndex = sourceIDToColumnIndex[sourceID];
        auto& column = (*this)[time];

        auto nblock = block;
        for (auto& nval : nblock) nval *= -1.0f;

        playbackData[time] = std::accumulate(column.begin() + 1, column.end(), Block(BlockSize, 1.0f) , [](const Block& prev, const Block& now) {
            Block c(BlockSize, 0.0f);
            std::transform(prev.begin(), prev.end(), now.begin(), c.begin(), std::plus<>());
            return c;
        });
    }


    Block& AudioMixerBlock::getBlock(int64_t time)
    {
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