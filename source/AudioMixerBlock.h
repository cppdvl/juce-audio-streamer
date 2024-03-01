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
    constexpr size_t BlockSize = 480;

    Block SubBlocks(const Block& a, const Block& b);
    Block AddBlocks(const Block& a, const Block& b);

    struct RowData
    {
        size_t size{0};
        int64_t minSampleIndex{-1};
        int64_t maxSampleIndex{-1};
        int64_t indexAccumulation{0};
    };
    /*!
     * @brief The AudioMixerBlock class.
     * @details This class is a mixer that can mix audio blocks from different sources. The AudioMixer is a map of bi-blocks, where each block is a pile of audio blocks from different sources, ata given time to provide syncronization.
     *
     *
     *
     * @note This class is thread safe.
     */
    class AudioMixerBlock : public std::map<TTime, Column>
    {
        std::recursive_mutex data_mutex;

        std::unordered_map<TUserID, size_t> sourceIDToColumnIndex {{0, 0}};
        Row playbackDataBlock {{0, Block (BlockSize, 0.0f)}};
        void layoutCheck(int64_t time, TUserID sourceID);

        /*!
         * @brief Add a source to the mixer.
         * @param sourceID
         */
        void addSource(TUserID sourceId);
        void addColumn(TTime time);

        /*!
         * @brief Add a audioBlock to the mixer, from any given source, at any given time.
         *
         * @param time The time in samples.
         * @param audioBlock The audio block to mix.
         * @param sourceID The source ID.
         * @param blocksToStream The blocks to stream.
         */
        void mix (
            TTime time,
            const Block audioBlock,
            TUserID sourceID);

        void replace(
            TTime time,
            const Block audioBlock,
            TUserID sourceID);
    public:
        AudioMixerBlock() : std::map<int64_t , Column>{
            {0, Column(1, Block(BlockSize, 0.0f))}}
        {}

        /*!
         * @brief Mix a block from a given source at a given time.
         * @param Vector Mixing Blocks (one per channel).
         * @param time The timestamp in samples.
         * @param splittedBlocks The blocks to mix.
         * @param sourceID The source ID. By default 0 if not provided (the local audio header).
         * @param emit If true, emit the mixFinished signal.
         */
        static void mix(
            std::vector<AudioMixerBlock>& mixers,
            int64_t time,
            const std::vector<Block>& splittedBlocks,
            Mixer::TUserID sourceID = 0,
            bool emit = true);
        /*!
         * @brief Replace a block in the playhead at given time. We use this in the peers, because we know we are going to receive one single signal.
         * @param Vector Mixing Blocks (one per channel).
         * @param time The timestamp in samples.
         * @param splittedBlocks The blocks to mix.
         * @param sourceID The source ID. By default 0 if not provided (the local audio header).
         * @param emit If true, emit the mixFinished signal.
         */
        static void replace(
            std::vector<AudioMixerBlock>& mixers,
            int64_t time,
            const std::vector<Block>& splittedBlocks,
            Mixer::TUserID sourceID = 0,
            bool emit = false);


        inline static DAWn::Events::Signal<std::vector<Mixer::Block>&, int64_t> mixFinished {};
        inline static DAWn::Events::Signal<std::vector<AudioMixerBlock>&, int64_t> invalidBlock{};
        /*!
         * @brief Get the block at a given time.
         *
         * @param time The time in samples.
         * @return The block at the given time.
         */
        Block getBlock(int64_t time);

        /*! Get the whole Row */
        const Row& getPlaybackDataBlock();

        /*!
         * @brief check if there's data in the audio playhead
         */
        bool hasData(int64_t time);
        inline bool hasNotData(int64_t time) { return !hasData(time); }
        /*!
         * @brief Check if we have a valid mixer.
         * @param mixers
         * @param time
         * @return
         */
        static bool invalid(std::vector<AudioMixerBlock>& mixers, int64_t time);
        inline static bool valid(std::vector<AudioMixerBlock>& mixers, int64_t time) { return !invalid(mixers, time); }

        void clear();
    };




}


#endif //AUDIOSTREAMPLUGIN_AUDIOMIXERBLOCK_H
