#include "PluginEditor.h"
#include "PluginProcessor.h"


#include <random>
#include <thread>
#include <cstddef>
#include <fstream>
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

    /* Check Account */
    std::ifstream file("/tmp/dawnaccount.txt"); // Replace "your_file.txt" with your file name
    if (file.is_open()) {

        if (getline(file, mAPIKey)) {
            std::cout << "akTkn: [ -- " << mAPIKey << " -- ]" << std::endl;
        } else {
            std::cout << "CRITICAL: File is empty or unable to read the akTkn." << std::endl;
        }
        file.close();
    } else {
        std::cout << "Unable to open file." << std::endl;
    }

    //TODO: I DISCOVER A BUG WHERE THIS IS CRASHING WHEN TRYING TO CONNECT A SECOND TIME.
    mWSApp.OnYouArePeer.Connect(this, &AudioStreamPluginProcessor::commandSetHost);
    mWSApp.OnYouAreHost.Connect(this, &AudioStreamPluginProcessor::commandSetPeer);
    mWSApp.OnDisconnectCommand.Connect(this, &AudioStreamPluginProcessor::commandDisconnect);
    mWSApp.ThisPeerIsGone.Connect(this, &AudioStreamPluginProcessor::peerGone);
    mWSApp.ThisPeerIsConnected.Connect(this, &AudioStreamPluginProcessor::peerConnected);
    mWSApp.OnSendAudioSettings.Connect(this, &AudioStreamPluginProcessor::commandSendAudioSettings);
    mWSApp.AckFromBackend.Connect(this, &AudioStreamPluginProcessor::backendConnected);
    std::thread([this](){mWSApp.Init(mAPIKey);}).detach();



    //ok this is the initialization routine for all objects:
    //Object 1. AUDIOMIXERS[2] => Two Audio Mixers. One for each channel. IM FORCING 2 CHANNELS. If other channels are involved Im ignoring them.
    //Object 2. RTPWRAP => RTPWrap object. This object is the one that handles the Network Interface.
    //Object 3. OPUSCODECMAP => Opus Codec. This object is the one that handles the encoding and decoding of the audio.
    mAudioMixerBlocks   = std::vector<Mixer::AudioMixerBlock>(static_cast<size_t>(getTotalNumInputChannels()));


    //Connection Direct
    if (!pRtp)
    {
        pRtp = std::make_unique<UDPRTPWrap>();


        //TODO: TEMPORAL
        std::string ip = "44.205.23.6";
        int port = 8899;

        rtpSessionID = pRtp->CreateSession(ip);
        rtpStreamID = pRtp->CreateStream(rtpSessionID, port, 2); //Direction (2), is ignored.

        auto pStream = _rtpwrap::data::GetStream(rtpStreamID);
        std::vector<std::byte> data(1, std::byte('.'));

        //bind a codec to the stream
        const OpusImpl::CODECConfig cfg;
        auto _pRtp = pRtp.get();
        auto _udpRtp = dynamic_cast<UDPRTPWrap*>(_pRtp);
        {
            auto userID = static_cast<Mixer::TUserID>(_udpRtp->GetUID());
            opusCodecMap.insert(std::make_pair(userID, OpusImpl::CODEC(cfg)));
        }

        pStream->letDataReadyToBeTransmitted.Connect(std::function<void(const std::string, std::vector<std::byte>&)>{[this](auto const, auto& refData){
            aboutToTransmit(refData);
        }});
        pStream->letDataFromPeerIsReady.Connect(std::function<void(uint64_t, std::vector<std::byte>&)>{
            [this](auto, auto& uid_ts_encodedPayload){

                if (uid_ts_encodedPayload.size() < 8)
                {
                    std::cout << "Bad formatting in the payload" << std::endl;
                    return;
                }

                //EXTRACT
                auto& source = uid_ts_encodedPayload;
                std::vector<std::byte> encodedPayLoad(
                    std::make_move_iterator(source.begin() + 8),
                    std::make_move_iterator(source.end()));
                std::vector<std::byte> nSample_ (
                    std::make_move_iterator(source.begin()+4),
                    std::make_move_iterator(source.end()));
                auto& userID_ = uid_ts_encodedPayload;

                auto ui32nSmpl  = *reinterpret_cast<uint32_t*>(nSample_.data());
                auto nSample    = static_cast<int64_t>(ui32nSmpl);
                auto userID     = *reinterpret_cast<Mixer::TUserID*>(userID_.data());

                //DECODE
                auto [_r, _p, _pS] = opusCodecMap[userID].decodeChannel(encodedPayLoad.data(), encodedPayLoad.size(), 0);
                auto& decodingResult        = _r;
                auto& decodedPayload        = _p;

                if (decodingResult != OpusImpl::Result::OK)
                {
                    std::cout << "Decoding Error" << _pS << std::endl;
                    return;
                }

                //MIX
                auto& interleavedBlocks = decodedPayload;
                std::vector<Mixer::Block> blocks{};
                Utilities::Data::deinterleaveBlocks(blocks, interleavedBlocks);
                Mixer::AudioMixerBlock::mix(mAudioMixerBlocks, nSample, blocks, userID);


        }});


        pStream->letThreadStarted.Connect(std::function<void(uint64_t)>{[](uint64_t peerId) {
            std::cout << peerId << " Thread Started" << std::endl;
        }});

        pStream->run();
    }

    //Audio Mixer Events
    Mixer::AudioMixerBlock::playbackHeadReady.Connect(std::function<void(std::vector<Mixer::Block>&)>{
        [this](auto& playbackHead){

            if (mRole == Role::NonMixer) return;

            //ENCODE and Send
            auto& deinterleavedBlocks = playbackHead;
            std::vector<std::vector<float>> interleavedBlocksVector {};
            Utilities::Data::interleaveBlocks(interleavedBlocksVector, deinterleavedBlocks);
            auto& interleavedBlocks = interleavedBlocksVector[0]; //Only 2 channels.
            auto pInterleavedBlocks = interleavedBlocks.data();
            auto uid = (dynamic_cast<UDPRTPWrap*>(pRtp.get()))->GetUID();
            auto [_r, _p, _pS] = opusCodecMap[uid].encodeChannel(pInterleavedBlocks, 0);
            auto& result = _r;
            if (result != OpusImpl::Result::OK)
            {
                std::cout << "MIXER: Encoding Error" << _pS << std::endl;
                return;
            }
            auto &payload = _p;
            pRtp->PushFrame(payload, rtpStreamID, 0);

        }
    });
}

bool& AudioStreamPluginProcessor::getMonoFlagReference()
{
    return mMonoSplit;
}

const int& AudioStreamPluginProcessor::getSampleRateReference() const
{
    return mSampleRate;
}

const int& AudioStreamPluginProcessor::getBlockSizeReference() const
{
    return mBlockSize;
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
    //mChannels = 2
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }
}

void AudioStreamPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer&)
{

    rmsLevels.first = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    rmsLevels.second = buffer.getRMSLevel(1, 0, buffer.getNumSamples());
    rmsLevels.first = juce::Decibels::gainToDecibels(rmsLevels.first);
    rmsLevels.second = juce::Decibels::gainToDecibels(rmsLevels.second);

    auto sound = std::min(rmsLevels.first, rmsLevels.second) >= -60.0f;
    if (!sound || mRole == Role::None) return;

    beforeProcessBlock(buffer);
    auto [nTimeMS, timeStamp] = getUpdatedTimePosition();

    std::vector<Mixer::Block> splittedBlocks {};
    std::vector<Mixer::Block> __interleavedBlocksVector {};

    if (mRole == Role::Mixer)
    {
        //MIXER BLOCK
        //Tx The Audio Playback
        splittedBlocks = std::vector<Mixer::Block>{mAudioMixerBlocks[0].getBlock(timeStamp), mAudioMixerBlocks[1].getBlock(timeStamp)};
        Mixer::AudioMixerBlock::mix(mAudioMixerBlocks, timeStamp, splittedBlocks);

    }
    else if (mRole == Role::NonMixer)
    {
        //NONMIXER
        //Get From Audio Buffer
        Utilities::Data::splitChannels(splittedBlocks, buffer, mMonoSplit);
        Utilities::Data::interleaveBlocks(__interleavedBlocksVector, splittedBlocks);
        auto& interleavedBlocks = __interleavedBlocksVector[0];
        auto pInterleavedBlocks = interleavedBlocks.data();
        auto uid = (dynamic_cast<UDPRTPWrap*>(pRtp.get()))->GetUID();
        auto [_r, _p, _ps] = opusCodecMap[uid].encodeChannel(pInterleavedBlocks, 0);
        auto& result = _r;
        if (result != OpusImpl::Result::OK)
        {
            std::cout << "NONMIXER: Encoding Error" << _ps << std::endl;
        }
        else
        {
            auto &payload = _p;
            pRtp->PushFrame(payload, rtpStreamID, static_cast<uint32_t>(timeStamp));
        }
    }

    if (mRole == Role::NonMixer){
        splittedBlocks.clear();
        splittedBlocks = std::vector<Mixer::Block>{mAudioMixerBlocks[0].getBlock(timeStamp), mAudioMixerBlocks[1].getBlock(timeStamp)};
    }

    //Join Over.
    if (mRole != Role::None) Utilities::Data::joinChannels(buffer, splittedBlocks);

}

void AudioStreamPluginProcessor::setRole(Role role)
{
    std::cout << "Role: " << (role == Role::Mixer ? "HOST: Mixer" : "PEER: NonMixer") << std::endl;
    mRole = role;
}
void AudioStreamPluginProcessor::commandSetHost(const char*)
{
    setRole(Role::Mixer);
}

void AudioStreamPluginProcessor::commandSetPeer (const char*)
{
    setRole(Role::NonMixer);
}

void AudioStreamPluginProcessor::commandDisconnect (const char*)
{
    std::cout << "FINISH THE PLUGIN AND THE DAW AND RESTART" << std::endl;
}

void AudioStreamPluginProcessor::peerGone (const char*)
{
    std::cout << "PEER GONE" << std::endl;
}

void AudioStreamPluginProcessor::peerConnected (const char*)
{
    std::cout << "PEER CONNECTED" << std::endl;
}

void AudioStreamPluginProcessor::commandSendAudioSettings (const char*)
{
    std::cout << "SENDING AUDIO SETTINGS:" << std::endl;
    auto j =  DAWn::Messages::AudioSettingsChanged(48000, 480, 32);
    auto pManager = mWSApp.pSm.get();
    auto settingsString = j.dump();
    dynamic_cast<DAWn::WSManager *>(pManager)->Send(settingsString);
}

void AudioStreamPluginProcessor::backendConnected (const char*)
{
    std::cout << "ACK FROM BACKEND" << std::endl;
}

void AudioStreamPluginProcessor::aboutToTransmit(std::vector<std::byte>& data)
{
    //Rationale, mixer host should be an even uid. Non mixer host should be an odd uid.
    if (mRole == Role::Mixer)   data[0] &= std::byte(0xFE);
    else                        data[0] |= std::byte(0x01);
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
