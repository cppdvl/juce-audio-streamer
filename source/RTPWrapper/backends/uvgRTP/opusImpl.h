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
    struct CODEC
    {
        int error{};
        int*const pError{&error};
        OpusEncoder* enc;
        OpusDecoder* dec;
        std::vector<SPEncoder> mEncs{};
        std::vector<SPDecoder> mDecs{};

        RTPStreamConfig cfg;
        CODEC(const RTPStreamConfig& _cfg) : cfg(_cfg)
        {

            if (cfg.mDirection == 0)
            {
                for (auto channelIndex = cfg.mChannels; channelIndex > 0; --channelIndex)
                {
                    auto pEncoder  = opus_encoder_create(cfg.mSampRate, cfg.mChannels, OPUS_APPLICATION_AUDIO, pError);
                    auto spEncoder = std::shared_ptr<OpusEncoder>(pEncoder, EncoderDeallocator());
                    mEncs.push_back(spEncoder);
                }
                enc = opus_encoder_create(cfg.mSampRate, cfg.mChannels, OPUS_APPLICATION_AUDIO, pError);
                dec = nullptr;
            }
            else
            {
                for (auto channelIndex = cfg.mChannels; channelIndex > 0; --channelIndex)
                {
                    auto pDecoder  = opus_decoder_create(cfg.mSampRate, cfg.mChannels, pError);
                    auto spDecoder = std::shared_ptr<OpusDecoder>(pDecoder, DecoderDeallocator());
                    mDecs.push_back(spDecoder);
                }
                enc = nullptr;
                dec = opus_decoder_create(cfg.mSampRate, cfg.mChannels, pError);
            }
        }
        ~CODEC()
        {
            if (enc != nullptr)
            {
                opus_encoder_destroy(enc);
            }
            if (dec != nullptr)
            {
                opus_decoder_destroy(dec);
            }
        }

        EncodingResult encodeChannel (float* pfPCM, const size_t channelIndex);
        DecodingResult decodeChannel (std::byte* pEncodedData, size_t channelSizeInBytes, const int channelIndex);
    };

}



#endif //AUDIOSTREAMPLUGIN_OPUSIMPL_H
