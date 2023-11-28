//
// Created by Julian Guarin on 7/11/23.
//

#ifndef AUDIOSTREAMPLUGIN_OPUSIMPL_H
#define AUDIOSTREAMPLUGIN_OPUSIMPL_H

#include "opus.h"
#include "RTPWrap.h"

#include <map>
#include <memory>
#include <iostream>


namespace OpusImpl
{
    struct CODEC;
    enum Result
    {
        OK,
        ERROR
    };

}
using SPEncoder         = std::shared_ptr<OpusEncoder>;
using SPDecoder         = std::shared_ptr<OpusDecoder>;
using SPCodec           = std::shared_ptr<OpusImpl::CODEC>;
using StrmIOCodec       = std::map<uint64_t, SPCodec>;
using EncodingResult    = std::tuple<OpusImpl::Result, std::vector<std::byte>, int>; // Result, EncodedData, EncodedDataSizeInBytes
using DecodingResult    = std::tuple<OpusImpl::Result, std::vector<std::byte>, int>; // Result

namespace OpusImpl
{

    struct EncoderDeallocator
    {
        void operator()(OpusEncoder * ptr)
        {
            if (ptr != nullptr)
            {
                opus_encoder_destroy(ptr);
            }
        }
    };
    struct DecoderDeallocator
    {
        void operator()(OpusDecoder * ptr)
        {
            if (ptr != nullptr)
            {
                opus_decoder_destroy(ptr);
            }
        }
    };
    struct CODECConfig
    {
        int32_t mSampRate{48000};
        int     mBlockSize{480};
        int     mChannels{2};
        bool    useStereoPair{true};
        bool    voice{false};
        CODECConfig() = default;
    };
    struct CODEC
    {
        int error{};
        int*const pError{&error};
        std::vector<SPEncoder> mEncs{};
        std::vector<SPDecoder> mDecs{};

        CODECConfig cfg;

        CODEC(const CODECConfig& _cfg) : cfg (_cfg)
        {
            bool odd = cfg.mChannels & 1;
            auto nCodecsToUse = !cfg.useStereoPair ? cfg.mChannels : cfg.mChannels >> 1;
            for (auto channelsLeft = 0; channelsLeft < nCodecsToUse; ++channelsLeft)
            {
                int channels = cfg.useStereoPair ? 2 : 1;
                auto application = cfg.voice ? OPUS_APPLICATION_VOIP : OPUS_APPLICATION_AUDIO;
                mEncs.push_back(std::shared_ptr<OpusEncoder>(opus_encoder_create(cfg.mSampRate, channels, application, pError), EncoderDeallocator()));
                mDecs.push_back(std::shared_ptr<OpusDecoder>(opus_decoder_create(cfg.mSampRate, channels, pError), DecoderDeallocator()));
            }
                    if (odd && cfg.useStereoPair)
            {
                int channels = 1;
                auto application = cfg.voice ? OPUS_APPLICATION_VOIP : OPUS_APPLICATION_AUDIO;
                mEncs.push_back(std::shared_ptr<OpusEncoder>(opus_encoder_create(cfg.mSampRate, channels, application, pError), EncoderDeallocator()));
                mDecs.push_back(std::shared_ptr<OpusDecoder>(opus_decoder_create(cfg.mSampRate, channels, pError), DecoderDeallocator()));
            }
        }
        ~CODEC()
        {
            mEncs.clear();
            mDecs.clear();
        }

        std::tuple<OpusImpl::Result, std::vector<std::byte>, size_t> encodeChannel (float* pfPCM, const size_t encoderIndex);
        std::tuple<OpusImpl::Result, std::vector<float>, size_t> decodeChannel (std::byte* pEncodedData, size_t channelSizeInBytes, const size_t channelIndex);
    };

}



#endif //AUDIOSTREAMPLUGIN_OPUSIMPL_H
