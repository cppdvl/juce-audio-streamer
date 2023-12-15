#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "Utilities/Utilities.h"

#include "opusImpl.h"

#include <random>
#include <thread>
#include <cstddef>
//==============================================================================

AudioStreamPluginProcessor::AudioStreamPluginProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
}

void AudioStreamPluginProcessor::prepareToPlay (double , int )
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    //Set 48k (More Suitable for Opus according to documentation)
    // I removed forcing the sample rate to 48k, because it was causing issues with the graphical interface and I had no certainty about the real size of the buffer and its duration.

    mAudioMixerBlocks   = std::vector<Mixer::AudioMixerBlock>(static_cast<size_t>(getTotalNumInputChannels()));

}

std::tuple<uint32_t, int64_t> AudioStreamPluginProcessor::getUpdatedTimePosition()
{
    auto [nTimeMS, nSamplePosition] = Utilities::Time::getPosInMSAndSamples(getPlayHead());
    jassert(nSamplePosition >= 0);
    return std::make_tuple(nTimeMS, nSamplePosition);
}



void AudioStreamPluginProcessor::beforeProcessBlock(juce::AudioBuffer<float>& buffer)
{
    mBlockSize = buffer.getNumSamples();
    mSampleRate = static_cast<int>(getSampleRate());
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    mChannels = totalNumOutputChannels - totalNumInputChannels;
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }
}

void AudioStreamPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    beforeProcessBlock(buffer);
    auto [nTimeMS, timeStamp] = getUpdatedTimePosition();


    std::vector<Mixer::Block> splittedBlocks {};
    Utilities::Data::splitChannels(splittedBlocks, buffer, mMonoSplit);

    //AUDIOMIXER BLOCK
    size_t channelIndex = 0;
    jassert(mAudioMixerBlocks.size() == splittedBlocks.size());
    Mixer::AudioMixerBlock::mix(mAudioMixerBlocks, timeStamp, splittedBlocks);



    //PLAYBACK (Only two channels)
    if (listenedRenderedAudioChannel) Utilities::Data::joinChannels(buffer, {mAudioMixerBlocks[0].getBlock(timeStamp), mAudioMixerBlocks[1].getBlock(timeStamp)});
    //TODO: Create a button to listen to rendered AudioChannel.

}


//==============================================================================
bool AudioStreamPluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioStreamPluginProcessor::createEditor()
{
    return new AudioStreamPluginEditor (*this);
}

//==============================================================================
void AudioStreamPluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioStreamPluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioStreamPluginProcessor();
}

//==============================================================================
const juce::String AudioStreamPluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioStreamPluginProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AudioStreamPluginProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AudioStreamPluginProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AudioStreamPluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioStreamPluginProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
        // so this should be at least 1, even if you're not really implementing programs.
}

int AudioStreamPluginProcessor::getCurrentProgram()
{
    return 0;
}

void AudioStreamPluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioStreamPluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioStreamPluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================


AudioStreamPluginProcessor::~AudioStreamPluginProcessor()
{
    /*if (streamSessionID)
    {
        pRTP->DestroySession(streamSessionID);
    }*/
}

void AudioStreamPluginProcessor::releaseResources()
{

}

bool AudioStreamPluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

        // This checks if the input layout matches the output layout
    #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
#endif
}


/*
AudioStreamPluginProcessor::AudioStreamPluginProcessor()
    : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
    #if ! JucePlugin_IsSynth
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
    #endif
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
      )
{

    if (!streamSessionID)
    {
        std::cout << "Failed to create the RTP Session" << std::endl;
    }
    else
    {
        streamIdOutput = pRTP->CreateStream(streamSessionID, outPort, 0);
        if (!streamIdOutput)
        {
            std::cout << "Failed to create an outbound RTP Stream" << std::endl;
        }
        else
        {
            std::cout << "Outbound Stream ID: " << streamIdOutput << std::endl;
        }

    std::vector<int32_t> ports{8888, 8889};

    bool searchingForListeningPort = true;
    for (auto& port : ports)
    {
        //wait
        inPort = port;
        outPort = 8888 + (port + 1) % 2;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 5);
        std::this_thread::sleep_for(std::chrono::seconds(dis(gen)));

        //check if port is in use or we are looking
        if (!searchingForListeningPort) break;
        if (RTPWrapUtils::udpPortIsInUse(port)) continue;

        //attempt to create an input stream
        streamIdInput = pRTP->CreateStream(streamSessionID, port, 1);
        auto pStream = UVGRTPWrap::GetSP(pRTP)->GetStream(streamIdInput);
        jassert(pStream && streamIdInput);
        {
            std::cout << "Inbound Stream ID: " << streamIdInput << std::endl;

            searchingForListeningPort = false;

            //Let's create the inbound stuff.
            auto installer = pStream->install_receive_hook (this, +[] (void* p, uvgrtp::frame::rtp_frame* pFrame) -> void {

                auto* pThis = reinterpret_cast<AudioStreamPluginProcessor*>(p);
                std::unordered_map<int32_t, std::vector<std::vector<float>>> streamIDToBlocks;

                //std::cout << "Received frame of size " << pFrame->payload_len << " bytes" << std::endl;
                auto timeStamp = pFrame->header.timestamp;


                //----------- START DECODING -------------
                //Convert frame into a vector of bytes, using the size of the payload
                //Decode into two interleaved blocks.
                auto [decResult, decodedIBlocks, decodedSamples] = pThis->pOpusCodec->decodeChannel(reinterpret_cast<std::byte*>(pFrame->payload), pFrame->payload_len, 0); //PEER 2 PEER

                auto blockSize_t = static_cast<size_t >(pThis->mBlockSize);
                jassert(blockSize_t);
                jassert(decodedSamples % blockSize_t == 0);
                //---------- DECODING DONE  ---------

                //---------- START MIXING ------------
                //Deinterleave the two blocks, into block0 and block1, one block per channel.
                std::vector<std::vector<float>> decodedDBlocks{};
                Utilities::Data::deinterleaveBlocks(decodedDBlocks, decodedIBlocks);

                for (auto audioMixerIndex = 0lu; audioMixerIndex < pThis->mAudioMixerBlocks.size(); ++audioMixerIndex)
                {
                    auto& audioMixerBlock = pThis->mAudioMixerBlocks[audioMixerIndex];
                    audioMixerBlock.mix(timeStamp, decodedDBlocks[audioMixerIndex], streamIDToBlocks, static_cast<int32_t>(pThis->streamIdOutput));
                }


                //Push the following (TS, OUTPUTSTREAMID, BLOCK0) into AudioMixer 0.
                //Push the following (TS, OUTPUTSTREAMID, BLOCK1) into AudioMixer 1.
                // ---------- MIXING DONE ----------
                for (auto& kv : streamIDToBlocks)
                {
                    auto streamOutID = static_cast<uint64_t>(kv.first);
                    auto& blocks = kv.second;

                    //From the resulting map interleave the two blocks.
                    std::vector<std::vector<float>>interleavedBlocks{};
                    Utilities::Data::interleaveBlocks(interleavedBlocks, blocks);
                    for (auto& interleavedBlock : interleavedBlocks)
                    {

                        //ENCODE
                        //Push the interleaved blocks into the OPUS ENCODER.
                        auto [result, encodedBlocks, encodedDataSizeInBytes] = pThis->pOpusCodec->encodeChannel(interleavedBlock.data(), 0); //PEER TO PEER

                        //PUSH FRAME
                        //Push the encoded data into the RTP stream thru Output Stream ID.
                        auto ptruvgrtp = UVGRTPWrap::GetSP(pThis->pRTP);

                        auto rtpResult = ptruvgrtp->GetStream(streamOutID)->push_frame(reinterpret_cast<uint8_t*>(encodedBlocks.data()), encodedDataSizeInBytes, timeStamp, RTP_NO_FLAGS);
                        if (rtpResult != RTP_OK)
                        {
                            std::cout << "Failed to push frame" << std::endl;
                        }
                    }
                }
            });

            if (installer != RTP_OK)
            {
                std::cout << "Failed to install receive hook" << std::endl;
                continue;
            }

            std::cout << "Ready to receive information!!!!" << std::endl;

            break;
        }
    }
}

}


void AudioStreamPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    beforeProcessBlock(buffer);
    auto [nTimeMS, timeStamp] = getUpdatedTimePosition();


    std::vector<std::vector<float>> splittedBlocks {};
    Utilities::Data::splitChannels(splittedBlocks, buffer);

    //AUDIOMIXER BLOCK
    std::unordered_map<int32_t, std::vector<Mixer::Block>> blocksToStream = pushDataIntoAudioMixerBlock (timeStamp, splittedBlocks);


    if(streamIdOutput)
    {
        auto ptruvgrtp = UVGRTPWrap::GetSP(pRTP);
        spStreamOut = ptruvgrtp->GetStream(streamIdOutput);

        for (auto& streamID_blocks : blocksToStream)
        {
            /*********************************************/
            /*********************************************/
            auto& mixedBlocksToStream = streamID_blocks.second;
            std::vector<Mixer::Block> iMixedBlocks{}; Utilities::Data::interleaveBlocks(iMixedBlocks, mixedBlocksToStream);

            //PEER TO PEER only the first from iMixedBlocks.
            auto& iMixedBlock = iMixedBlocks[0];

            //ENCODE
            //Push the interleaved blocks into the OPUS ENCODER.
            auto encoderIndex = 0lu;
            auto [result, encodedBlocks, encodedDataSizeInBytes] = pOpusCodec->encodeChannel(iMixedBlock.data(), encoderIndex);
            if (result != OpusImpl::Result::OK)
            {
                std::cout << "Failed to encode channel idx: " << 0 << std::endl;
                continue;
            }

            //PUSH FRAME
            //Push the encoded data into the RTP stream thru Output Stream ID.
            auto rtpResult = spStreamOut->push_frame(reinterpret_cast<uint8_t*>(encodedBlocks.data()), encodedDataSizeInBytes, static_cast<uint32_t >(timeStamp), RTP_NO_FLAGS);
            if (rtpResult != RTP_OK)
            {
                std::cout << "ERROR PUSHING THRU OUT STREAM" << std::endl;
            }
        }
    }

    //PLAYBACK (Only two channels)
    Utilities::Data::joinChannels(buffer, {mAudioMixerBlocks[0].getBlock(timeStamp), mAudioMixerBlocks[1].getBlock(timeStamp)});

}
*/

