//
// Created by Julian Guarin on 17/11/23.
//

#ifndef AUDIOSTREAMPLUGIN_AUDIOMIXERBLOCK_H
#define AUDIOSTREAMPLUGIN_AUDIOMIXERBLOCK_H

#include <map>
#include <mutex>
#include <vector>
#include <unordered_map>

#include "Events.h"

namespace Mixer
{
    using TTime = int64_t;
    using Block = std::vector<float>;
    using Column = std::vector<Block>;
    using Row = std::map<TTime, Block>;
    using TUserID = uint32_t;

    Block SubBlocks(const Block& a, const Block& b);
    Block AddBlocks(const Block& a, const Block& b);


    class AudioMixerBlock : public std::unordered_map<TTime, Column>
    {
        size_t mBlockSize{480};
        size_t mDeltaBlocks{100};
        std::recursive_mutex data_mutex;
        std::unordered_map<TUserID, size_t> sourceIDToColumnIndex {{0, 0}};
        Row playbackDataBlock {};

        void layoutCheck(int64_t time, TUserID sourceID);
        void addSource(TUserID sourceId);
        void addColumn(TTime time);
        void mix(TTime time, const Block audioBlock, TUserID sourceID);
        void replace(TTime time, const Block audioBlock, TUserID sourceID);

        void flushMixer();
        void resetMixer(size_t blockSize, uint32_t delayInSeconds = 0);
        //OPERATIONAL CONFIGURATION SECTION

        Block getBlock(const int64_t time, int64_t& realtime, bool delayed = true);
        static std::vector<Mixer::Block> getBlocks_(std::vector<AudioMixerBlock>& mixers, const int64_t time, int64_t& realtime, bool delayed = true)
        {
            return std::vector<Mixer::Block>{
                mixers[0].getBlock(time, realtime, delayed), mixers[1].getBlock(time, realtime, delayed)
            };
        }
        bool containsTimeStamp(const int64_t time);

    public:
        AudioMixerBlock(){}

        //MIX & REPLACE SECTION
        static void mix(
            std::vector<AudioMixerBlock>& mixers,
            int64_t time,
            const std::vector<Block>& splittedBlocks,
            Mixer::TUserID sourceID);

        static void replace(
            std::vector<AudioMixerBlock>& mixers,
            int64_t time,
            const std::vector<Block>& splittedBlocks,
            Mixer::TUserID sourceID = 0);



        static void resetMixers(std::vector<AudioMixerBlock>& mixers, size_t blockSize, uint32_t delayInSeconds = 0);
        static std::vector<Mixer::Block> getBlocksDelayed(std::vector<AudioMixerBlock>& mixers, const int64_t time, int64_t& realtime)
        {
            return getBlocks_(mixers, time, realtime, true);
        }
        static std::vector<Mixer::Block> getBlocks(std::vector<AudioMixerBlock>& mixers, const int64_t time, int64_t& realtime)
        {
            return getBlocks_(mixers, time, realtime, false);
        }

        static bool containsTimeStamp(std::vector<AudioMixerBlock>& mixers, const int64_t time);


        inline static DAWn::Events::Signal<std::vector<Mixer::Block>, int64_t> mixFinished {};
        inline static DAWn::Events::Signal<std::vector<AudioMixerBlock>&, int64_t> invalidBlock{};
        inline static DAWn::Events::Signal<size_t, size_t> replacingBlockMismatch{};


    };




}


#endif //AUDIOSTREAMPLUGIN_AUDIOMIXERBLOCK_H
