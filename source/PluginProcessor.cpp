#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Utilities/Configuration/Configuration.h"
#include <array>
#include <random>
#include <thread>
#include <cstddef>
#include <fstream>

#include <unistd.h>
//==============================================================================

AudioStreamPluginProcessor::AudioStreamPluginProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
      DAWn::Utilities::Configuration(".config/dawnaudio/init.dwn")
{
    if (transport.role != "none") mRole = transport.role == "mixer" || transport.role == "MIXER" ? Role::Mixer : Role::NonMixer;
    mAPIKey = auth.key;
}

void AudioStreamPluginProcessor::prepareToPlay (double , int )
{
    std::call_once(mOnceFlag, [this](){

        //INIT THE ROLE:
        mUserID         = 0;
        std::cout << "Process ID: " << getpid() << std::endl;


        //INITALIZATION LIST
        //Object 0. WEBSOCKET.
        //Object 1. AUDIOMIXERS[2] => Two Audio Mixers. One for each channel. IM FORCING 2 CHANNELS. If other channels are involved Im ignoring them.
        //Object 2. RTPWRAP => RTPWrap object. This object is the one that handles the Network Interface.
        //Object 3. OpusCodecMap => Errors assoc with this map.

        //OBJECT 0. WEBSOCKET COMMANDS
        mWSApp.OnYouAreHost.Connect(this, &AudioStreamPluginProcessor::commandSetHost);
        mWSApp.OnYouArePeer.Connect(this, &AudioStreamPluginProcessor::commandSetPeer);
        mWSApp.OnDisconnectCommand.Connect(this, &AudioStreamPluginProcessor::commandDisconnect);
        mWSApp.ThisPeerIsGone.Connect(this, &AudioStreamPluginProcessor::peerGone);
        mWSApp.ThisPeerIsConnected.Connect(this, &AudioStreamPluginProcessor::peerConnected);
        mWSApp.OnSendAudioSettings.Connect(this, &AudioStreamPluginProcessor::commandSendAudioSettings);
        mWSApp.AckFromBackend.Connect(this, &AudioStreamPluginProcessor::backendConnected);
        mWSApp.ApiKeyAuthFailed.Connect(std::function<void()>{[](){ std::cout << "API AUTH FAILED." << std::endl;}});
        mWSApp.ApiKeyAuthSuccess.Connect(std::function<void()>{[](){ std::cout << "API AUTH SUCCEEDED." << std::endl;}});

        //OBJECT 1. AUDIO MIXER
        mAudioMixerBlocks   = std::vector<Mixer::AudioMixerBlock>(audio.channels);

        //Audio Mixer Events
        Mixer::AudioMixerBlock::invalidBlock.Connect(std::function<void(std::vector<Mixer::AudioMixerBlock>&, int64_t)>{
            [](auto& mixerBlocks, auto timeStamp64)
            {
                auto timeStamp = static_cast<uint32_t>(timeStamp64);
                std::cout << "Invalid Block" << timeStamp << std::endl;
                for (size_t idx = 0; idx < mixerBlocks.size(); ++idx)
                {
                    std::cout << "Block [" << idx << "] : [" << mixerBlocks[idx].getBlock(timeStamp).size() << "]"<< std::endl;
                }
            }
        });

        Mixer::AudioMixerBlock::mixFinished.Connect(std::function<void(std::vector<Mixer::Block>&, int64_t)>{
            [this](auto& playbackHead, auto timeStamp64){
                if (mRole != Role::Mixer) return;
                packEncodeAndPush (playbackHead, static_cast<uint32_t> (timeStamp64));
            }
        });


        //OBJECT 3. OPUS CODEC MAP, error handling.
        OpusImpl::CODEC::sEncoderErr.Connect(std::function<void(uint32_t, const char*, float*)>{
            [](auto uid, auto err, auto pdata){
                std::cout << "Encoder Error for UID[" << uid << "] :" << err << std::endl;
                std::cout << "@" << std::hex << pdata << std::dec << std::endl;
            }
        });
        OpusImpl::CODEC::sDecoderErr.Connect(std::function<void(uint32_t, const char*, std::byte*)>{
            [](auto uid, auto err, auto pdata){
                std::cout << "Decoder Error for UID[" << uid << "] :" << err << std::endl;
                std::cout << "@" << std::hex << pdata << std::dec << std::endl;
            }
        });

        if (mRole != Role::None) startRTP(transport.ip, transport.port);

    });
    std::cout << "Preparing to play " << std::endl;

}

bool& AudioStreamPluginProcessor::getMonoFlagReference()
{
    return mAudioSettings.mMonoSplit;
}

const int& AudioStreamPluginProcessor::getSampleRateReference() const
{
    return mAudioSettings.mSampleRate;
}

const int& AudioStreamPluginProcessor::getBlockSizeReference() const
{
    return mAudioSettings.mDAWBlockSize;
}

void AudioStreamPluginProcessor::tryApiKey(const std::string& secret)
{
    std::thread(
        [this, secret](){
            mWSApp.Init(secret, auth.authEndpoint, auth.wsEndpoint);
        }).detach();
}
std::tuple<uint32_t, int64_t> AudioStreamPluginProcessor::getUpdatedTimePosition()
{
    auto [nTimeMS, nSamplePosition] = Utilities::Time::getPosInMSAndSamples(getPlayHead());
    jassert(nSamplePosition >= 0);
    return std::make_tuple(nTimeMS, nSamplePosition);
}



void AudioStreamPluginProcessor::beforeProcessBlock(juce::AudioBuffer<float>& buffer)
{
    auto dawReportedBlockSize = buffer.getNumSamples();
    if (mAudioSettings.mDAWBlockSize != dawReportedBlockSize)
    {
        mAudioSettings.mDAWBlockSize = dawReportedBlockSize;
        std::cout << "DAW Reported Block Size: " << mAudioSettings.mDAWBlockSize << std::endl;
    }

    auto dawReportedSampleRate = static_cast<int>(getSampleRate());
    if (mAudioSettings.mSampleRate != dawReportedSampleRate)
    {
        mAudioSettings.mSampleRate = dawReportedSampleRate;
        std::cout << "DAW Reported Sample Rate: " << mAudioSettings.mSampleRate << std::endl;
    }

    rmsLevelsInputAudioBuffer.first = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    rmsLevelsInputAudioBuffer.second = buffer.getRMSLevel(1, 0, buffer.getNumSamples());
    rmsLevelsInputAudioBuffer.first = juce::Decibels::gainToDecibels(rmsLevelsInputAudioBuffer.first);
    rmsLevelsInputAudioBuffer.second = juce::Decibels::gainToDecibels(rmsLevelsInputAudioBuffer.second);

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }
}

std::pair<OpusImpl::CODEC, std::vector<Utilities::Buffer::BlockSizeAdapter>>& AudioStreamPluginProcessor::getCodecPairForUser(Mixer::TUserID userID)
{
    if (mOpusCodecMap.find(userID) == mOpusCodecMap.end())
    {
        mOpusCodecMap[userID].first.cfg.ownerID = userID;
        jassert(mOpusCodecMap.find(userID) != mOpusCodecMap.end());
        jassert(mOpusCodecMap[userID].first.cfg.ownerID == userID);
        jassert(mOpusCodecMap[userID].first.mEncs.size() < 16);
        jassert(mOpusCodecMap[userID].first.mDecs.size() < 16);

        auto nOfSizeAdaptersInOneDirection = (audio.channels >> 1) + (audio.channels % 2);
        mOpusCodecMap[userID].second = std::vector<Utilities::Buffer::BlockSizeAdapter>(
            2 * nOfSizeAdaptersInOneDirection,                      // 2 because 1 set is outlet and the other inlet.
            Utilities::Buffer::BlockSizeAdapter(audio.bsize*2));    //Remember this is a channel pair per block / per codec.
    }

    return mOpusCodecMap[userID];
}

void AudioStreamPluginProcessor::extractDecodeAndMix(std::vector<std::byte>& uid_ts_encodedPayload)
{
    auto interleavedDAWBlockSize = 2 * mAudioSettings.mDAWBlockSize;
    auto [result, userID, nSample, encodedPayLoad] = Utilities::Buffer::extractIncomingData(uid_ts_encodedPayload);
    if (result == false) std::cout << "Error: Buffer Extraction" << std::endl;

    auto& [codec, blockSzAdapters]      = getCodecPairForUser(userID);
    auto& bsaInput = blockSzAdapters[1]; //This is the input channel.

    //TODO: OPTIMIZE THIS.
    bsaInput.setOutputBlockSize(2 * mAudioSettings.mDAWBlockSize);

    //DECODE
    auto [_r, _p, _pS]      = codec.decodeChannel(encodedPayLoad.data(), encodedPayLoad.size(), 0);
    auto& decodingResult    = _r;
    auto& decodedPayload    = _p;

    if (decodingResult != OpusImpl::Result::OK)
    {
        std::cout << "Decoding Error" << _pS << std::endl;
        return;
    }

    //TODO: THIS NEEDS TO BE IN A SEPARATE THREAD.
    bsaInput.push(decodedPayload);
    while (bsaInput.dataReady())
    {
        //MIX
        std::vector<Mixer::Block> blocks{};
        std::vector<float> interleavedAdaptedBlock(interleavedDAWBlockSize, 0.0f);
        bsaInput.pop(interleavedAdaptedBlock);
        Utilities::Buffer::deinterleaveBlocks(blocks, interleavedAdaptedBlock);
        if (mRole == Role::Mixer) Mixer::AudioMixerBlock::mix(mAudioMixerBlocks, nSample, blocks, userID);
        else if (mRole == Role::NonMixer) Mixer::AudioMixerBlock::replace(mAudioMixerBlocks, nSample, blocks, userID);
    }
}

void AudioStreamPluginProcessor::packEncodeAndPush(std::vector<Mixer::Block>& blocks, uint32_t timeStamp, bool retransmission)
{

    //dynamic cast to UDPRTPWrap
    auto _udpRtp = dynamic_cast<UDPRTPWrap*>(pRtp.get());
    if (retransmission && options.opuscache) if (_udpRtp->__dataIsCached(mRtpStreamID, timeStamp)) return;

    std::vector<Mixer::Block> __interleavedBlocks{};
    Utilities::Buffer::interleaveBlocks(__interleavedBlocks, blocks);
    auto& interleavedBlocks = __interleavedBlocks[0]; // This is the first pair of interleaved blocks. We are only using 2 channels.
    auto& [codec, blockSzAdapters] = getCodecPairForUser(mUserID);
    blockSzAdapters[0].push(interleavedBlocks);

    while (blockSzAdapters[0].dataReady())
    {
        std::vector<float> interleavedAdaptedBlock(audio.bsize * 2, 0.0f);
        blockSzAdapters[0].pop(interleavedAdaptedBlock);
        auto [_r, _p, _pS] = codec.encodeChannel(interleavedAdaptedBlock.data(), 0);
        auto& result = _r;
        if (result != OpusImpl::Result::OK)
        {
            std::cout << "Encoding Error" << _pS << std::endl;
            return;
        }
        auto &payload = _p;
        pRtp->PushFrame(payload, mRtpStreamID, timeStamp);
        _udpRtp->__cacheData(timeStamp, payload);
    }


}
void AudioStreamPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer&)
{
    beforeProcessBlock(buffer);

    auto sound = debug.overridermssilence || (std::min(rmsLevelsInputAudioBuffer.first, rmsLevelsInputAudioBuffer.second) >= -60.0f);
    if (!sound) return;

    auto roleset = mRole != Role::None || debug.requiresrole == false;
    if (!roleset) return;

    auto [nTimeMS, timeStamp64] = getUpdatedTimePosition();
    jassert(timeStamp64 % mAudioSettings.mDAWBlockSize == 0);

    std::vector<Mixer::Block> splittedBuffer{};
    std::vector<Mixer::Block> splittedPlayHead{};

    Utilities::Buffer::splitChannels(splittedBuffer, buffer, mAudioSettings.mMonoSplit);

    if (mRole == Role::Mixer)
    {
        Mixer::AudioMixerBlock::mix(mAudioMixerBlocks, timeStamp64, splittedBuffer, mUserID);
    }
    else
    {
        packEncodeAndPush (splittedBuffer, static_cast<uint32_t> (timeStamp64));
    }

    if (Mixer::AudioMixerBlock::valid(mAudioMixerBlocks, timeStamp64))
    {
        Utilities::Buffer::joinChannels(buffer, std::vector<Mixer::Block>{mAudioMixerBlocks[0].getBlock(timeStamp64), mAudioMixerBlocks[1].getBlock(timeStamp64)});
        rmsLevelsJitterBuffer.first = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
        rmsLevelsJitterBuffer.second = buffer.getRMSLevel(1, 0, buffer.getNumSamples());
        rmsLevelsJitterBuffer.first = juce::Decibels::gainToDecibels(rmsLevelsInputAudioBuffer.first);
        rmsLevelsJitterBuffer.second = juce::Decibels::gainToDecibels(rmsLevelsInputAudioBuffer.second);
    }
    else if (mRole != Role::NonMixer)
    {
        rmsLevelsJitterBuffer = std::make_pair(-60.0f, -60.0f);
        buffer.clear();
    }

}

void AudioStreamPluginProcessor::setRole(Role role)
{
    std::cout << "Role: " << (role == Role::Mixer ? "HOST: Mixer" : "PEER: NonMixer") << std::endl;
    mRole = role;
    sgnStatusSet.Emit(mRole == Role::Mixer ? "MIXER" : "PEER");
}
void AudioStreamPluginProcessor::startRTP(std::string ip, int port)
{
    //OBJECT 2. RTWRAP
    if (!pRtp)
    {
        pRtp = std::make_unique<UDPRTPWrap>();

        //TODO: TEMPORAL

        mRtpSessionID   = pRtp->CreateSession (ip);
        mRtpStreamID    = pRtp->CreateStream (mRtpSessionID, port, 2); //Direction (2), is ignored.
        auto _pRtp      = pRtp.get();
        auto _udpRtp    = dynamic_cast<UDPRTPWrap*> (_pRtp);
        {
            mUserID = static_cast<Mixer::TUserID> (_udpRtp->GetUID());
            getCodecPairForUser (mUserID);
        }

        auto pStream = _rtpwrap::data::GetStream (mRtpStreamID);

        //bind a codec to the stream

        pStream->letDataReadyToBeTransmitted.Connect (std::function<void (const std::string, std::vector<std::byte>&)> {
            [this] (auto const, auto& refData) {
                aboutToTransmit (refData);
            } });
        pStream->letDataFromPeerIsReady.Connect (std::function<void (uint64_t, std::vector<std::byte>&)> {
            [this] (auto, auto& uid_ts_encodedPayload) {
                extractDecodeAndMix (uid_ts_encodedPayload);
            } });
        pStream->letOperationalError.Connect (std::function<void (uint64_t, std::string)> { [] (uint64_t peerId, std::string error) {
            std::cout << "Socket operational error: " << peerId << " " << error << std::endl;
        } });

        pStream->letThreadStarted.Connect (std::function<void (uint64_t)> { [] (uint64_t peerId) {
            std::cout << peerId << " Thread Started" << std::endl;
        } });

        pStream->run();

        auto payload = std::vector<std::byte> (1, std::byte { 0x0 });
        pRtp->PushFrame (payload, mRtpStreamID, 0);
    }
}

void AudioStreamPluginProcessor::commandSetHost(const char*)
{
    setRole(Role::Mixer);
    startRTP("44.205.23.6", 8899);
}

void AudioStreamPluginProcessor::commandSetPeer (const char*)
{
    setRole(Role::NonMixer);
    startRTP("44.205.23.6", 8899);
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
