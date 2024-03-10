//
// Created by Julian Guarin on 17/11/23.
//

#ifndef AUDIOSTREAMPLUGIN_AUDIOMIXERBLOCK_H
#define AUDIOSTREAMPLUGIN_AUDIOMIXERBLOCK_H

#include <map>
#include <mutex>
#include <vector>
#include <unordered_map>

#include "signalsslots.h"

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
        size_t mDeltaBlocks{1000};
        std::recursive_mutex data_mutex;
        std::unordered_map<TUserID, size_t> sourceIDToColumnIndex {{0, 0}};
        Row playbackDataBlock {};

        void layoutCheck(int64_t time, TUserID sourceID);
        void addSource(TUserID sourceId);
        void addColumn(TTime time);
        void mix (TTime time, const Block audioBlock, TUserID sourceID);
        void replace(TTime time, const Block audioBlock, TUserID sourceID);
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


        //OPERATIONAL CONFIGURATION SECTION
        void setDeltaBlocks(size_t deltaBlocks)
        {
            mDeltaBlocks = deltaBlocks;
        }

        void flushMixer();
        void resetMixer(size_t blockSize);
        static void resetMixers(std::vector<AudioMixerBlock>& mixers, size_t blockSize);

        //GET DATA SECTION
        Block getBlock(const int64_t time, int64_t& realtime, bool delayed = true);

        //HARD VALIDATIONS
        bool hasData(int64_t time);
        inline bool hasNotData(int64_t time) { return !hasData(time); }
        static bool invalid(std::vector<AudioMixerBlock>& mixers, int64_t time);
        inline static bool valid(std::vector<AudioMixerBlock>& mixers, int64_t time) { return !invalid(mixers, time); }

        inline static DAWn::Events::Signal<std::vector<Mixer::Block>, int64_t> mixFinished {};
        inline static DAWn::Events::Signal<std::vector<AudioMixerBlock>&, int64_t> invalidBlock{};

    };




}


#endif //AUDIOSTREAMPLUGIN_AUDIOMIXERBLOCK_H
