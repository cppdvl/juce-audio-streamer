#include "PluginProcessor.h"
#include "PluginEditor.h"

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

    //Create a stream session
    pRTP->Initialize();

    //Create a stream session : this should change, the intention of this code is to purely test.
    streamSessionID = pRTP->CreateSession("127.0.0.1");
    if (!streamSessionID) {
        std::cout << "Failed to create session" << std::endl;
    }
    else
    {
        //try to create a listening stream (an input only stream).
        std::vector<int> ports {8888, 8889};

        bool searchPort = true;
        for (auto port : ports)
        {
            if (!searchPort) break;

            bool portIsNotFreeKeepLooking = !RTPWrapUtils::udpPortIsFree(port);
            if (portIsNotFreeKeepLooking || searchPort == false) continue;
            searchPort = false;
            inPort = port;
            outPort = inPort == 8888 ? 8889 : 8888;

            streamIdInput = pRTP->CreateStream(streamSessionID, inPort, 1);
            if (streamIdInput)
            {
                auto pStream = UVGRTPWrap::GetSP(pRTP)->GetStream(streamIdInput);
                if (!pStream)
                {
                    std::cout << "Failed to get stream: " << streamIdInput << std::endl;
                }
                else if (pStream->install_receive_hook(this, +[](void*p, uvgrtp::frame::rtp_frame* pFrame) -> void
                             {
                                 auto pThis = reinterpret_cast<AudioStreamPluginProcessor*>(p);
                                 std::lock_guard<std::mutex> lock (pThis->mMutexInput);
                                 for (auto sz = 0ul; sz < pFrame->payload_len; ++sz)
                                 {
                                     pThis->mInputBuffer.push_back(*reinterpret_cast<std::byte*>(pFrame->payload + sz));
                                 }
                                 uvgrtp::frame::dealloc_frame(pFrame);
                             }) != RTP_OK)
                {
                    std::cout << "Failed to install RTP receive hook!" << std::endl;
                }
                std::cout << "Stream ID Input: " << streamIdInput << std::endl;

                /*streamIdOutput = pRTP->CreateStream(streamSessionID, outPort, 0);
                if (!streamIdOutput)
                {
                    std::cout << "Failed to create out stream" << std::endl;
                }
                else
                {
                    std::cout << "Stream ID Output: " << streamIdOutput << std::endl;
                }*/
            }
        }
        if (searchPort)
        {
            std::cout << "Failed to find a free port." << std::endl;
        }
    }

}

AudioStreamPluginProcessor::~AudioStreamPluginProcessor()
{
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
void AudioStreamPluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    toneGenerator.prepareToPlay(samplesPerBlock, sampleRate);
    channelInfo.buffer = nullptr;
    channelInfo.startSample = 0;


}

void AudioStreamPluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    toneGenerator.releaseResources();
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
void AudioStreamPluginProcessor::processBlockStreamInNaive(
    juce::AudioBuffer<float>& buffer,
        juce::MidiBuffer&)
{

    auto naiveUnPackErrorControl = [this]() -> std::tuple<int, int, int, std::vector<float>, juce::AudioBuffer<float>> {

        std::lock_guard<std::mutex> lock (mMutexInput);
        auto naiveUnPack = [this](void* ptr, int byteSize) -> bool
        {
            auto p = reinterpret_cast<std::byte*>(ptr);
            for (auto sz = 0l; sz < byteSize; ++sz)
            {
                if (mInputBuffer.empty()) return false;
                *reinterpret_cast<std::byte*>(p + sz) = mInputBuffer.front();
                mInputBuffer.pop_front();
            }
            return true;
        };

        int nChan = 0;
        int nSampl  = 0;
        int nECtrl  = 0;
        if (!naiveUnPack (&nChan, sizeof (int)))
            return make_tuple (0, 0, 0, std::vector<float> {}, juce::AudioBuffer<float> {});
        if (!naiveUnPack (&nSampl, sizeof (int)))
            return make_tuple (0, 0, 0, std::vector<float> {}, juce::AudioBuffer<float> {});
        if (!naiveUnPack (&nECtrl, sizeof (int)))
            return make_tuple (0, 0, 0, std::vector<float> {}, juce::AudioBuffer<float>{});

        int nECtrl_ = nChan + nSampl;
        int minBufferSizeExpected = nSampl * nChan;
        int totalBufferSize = (int)mInputBuffer.size();

        if (nECtrl_ != nECtrl || minBufferSizeExpected > totalBufferSize)
            return make_tuple (0, 0, 0, std::vector<float> {}, juce::AudioBuffer<float> {});

        int bufferSize = nSampl * nChan;

        std::vector<float> f ((size_t)bufferSize, 0.0f);
        juce::AudioBuffer<float> inStreamBuffer(nChan, nSampl);
        std::vector<float*> channelPtrs((size_t)nChan, nullptr);
        for (auto index = 0; index < nChan; ++index) channelPtrs[(size_t)index] = inStreamBuffer.getWritePointer (index);
        for (auto bufferIndex = 0; nChan > 0 && bufferIndex < bufferSize; ++bufferIndex)
        {
            int channelIndex = bufferIndex / nSampl;
            jassert(channelIndex < nChan);
            int sampleIndex = bufferIndex % nSampl;

            float fValue = 0.0f;
            if (!naiveUnPack (&fValue, sizeof (float)))
                return make_tuple (0, 0, 0, std::vector<float> {}, juce::AudioBuffer<float> {});
            channelPtrs[(size_t)channelIndex][sampleIndex] = fValue;
            f[(size_t) sampleIndex] = fValue;
        }
        inStreamBuffer.applyGain (nChan > 0 ? static_cast<float>(streamInGain) : 0.0f);
        return make_tuple (nChan, nSampl, nECtrl, f, inStreamBuffer);
    };
    auto [nOChan, nSampl, nECtrl, f, streamInBuffer] = naiveUnPackErrorControl();

    for (int channel = 0; channel < nOChan; ++channel)
    {
        if (!gettingData)
        {
            std::cout << std::this_thread::get_id() << " Getting Data." << std::endl;
            gettingData = true;
        }

        auto bufferChannelData = buffer.getWritePointer (channel);
        auto streamInBufferChannelData = streamInBuffer.getReadPointer (channel);
        for (int sample = 0; sample < nSampl; ++sample)
        {
            //Naive Mix.
            bufferChannelData[sample] += streamInBufferChannelData[sample];
        }
    }
    if (!nOChan)
    {
        //Tone Generator
        if (gettingData)
        {
            std::cout << std::this_thread::get_id()  << " NOT Getting Data." << std::endl;
            gettingData = false;
        }

        auto totalNumberOfSamples= buffer.getNumSamples();
        channelInfo.buffer = &buffer;
        channelInfo.numSamples = totalNumberOfSamples;
        toneGenerator.getNextAudioBlock(channelInfo);
    }

}
void AudioStreamPluginProcessor::processBlockStreamOutNaive (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    if (!streamIdOutput)
    {
        streamIdOutput = pRTP->CreateStream(streamSessionID, outPort, 0);
        if (!streamIdOutput) return;
    }
    //Tone Generator
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto totalNumberOfSamples   = buffer.getNumSamples();

    std::vector<std::byte> bNaive {};
    auto outStreamBuffer = juce::AudioBuffer<float>{totalNumOutputChannels, totalNumberOfSamples};


    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto naivePack = [&bNaive, this](void* rawReadPointer, int bytesize, bool applyGain = false) -> void
        {
            auto p = reinterpret_cast<std::byte*>(rawReadPointer);
            size_t step = applyGain ? sizeof (float) : sizeof (std::byte);
            for(auto sz = 0l; sz < bytesize; sz += step)
            {
                if (applyGain)
                {
                    auto f = *reinterpret_cast<float*>(p + sz);
                    f *= static_cast<float>(streamOutGain);
                    auto bPtr = reinterpret_cast<std::byte*>(&f);

                    for (size_t i = 0; i < step; ++i)
                    {
                        auto byte = bPtr[i];
                        bNaive.push_back (byte);
                    }

                }
                else
                {
                    bNaive.push_back (*reinterpret_cast<std::byte*> (p + sz));
                }
            }

        };
        if (!channel)
        {
            auto naiveErrorControl = totalNumberOfSamples + totalNumInputChannels;

            naivePack(&totalNumInputChannels, sizeof(int));
            naivePack(&totalNumberOfSamples, sizeof(int));
            naivePack(&naiveErrorControl, sizeof(int));

            outStreamBuffer = buffer;
            outStreamBuffer.applyGain(static_cast<float> (streamOutGain));
        }

        int szSamples = totalNumberOfSamples * (int)sizeof(float);
        naivePack(const_cast<float*>(outStreamBuffer.getReadPointer (channel)), szSamples, true);
        if (channel == totalNumOutputChannels - 1)
            streamOutNaive(8888, bNaive);

    }
}
void AudioStreamPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    if (auto* playHead = getPlayHead())
    {
        auto optionalPositionInfo = playHead->getPosition();
        if (optionalPositionInfo)
        {
            juce::Optional<double> info = optionalPositionInfo->getPpqPosition();
            mScrubCurrentPosition.store(*info);
        }
    }

    juce::ScopedNoDenormals noDenormals;

    //Tone Generator
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }

    /*processBlockStreamInNaive(buffer, midiMessages);*/
    channelInfo.buffer = &buffer;
    channelInfo.numSamples = buffer.getNumSamples();
    toneGenerator.getNextAudioBlock(channelInfo);

    /*processBlockStreamOutNaive(buffer, midiMessages);*/
    buffer.applyGain(static_cast<float> (masterGain));
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

std::once_flag noSessionIDflag;
void AudioStreamPluginProcessor::streamOutNaive (int remotePort, std::vector<std::byte> data)
{
    if (!streamSessionID)
    {
        std::call_once(noSessionIDflag, []() { std::cout << "CRITICAL ERROR: No Session Was Created." << std::endl; });
        return;
    }
    if (!streamIdOutput)
    {
        streamIdOutput = pRTP->CreateStream(streamSessionID, remotePort, 0);
        if (!streamIdInput)
        {
            std::cout << "Failed to create stream" << std::endl;
            return;
        }
    }
    auto pmedia = reinterpret_cast<uint8_t *>(data.data());
    auto pStream = UVGRTPWrap::GetSP(pRTP)->GetStream(streamIdInput);
    if (pStream->push_frame(pmedia, data.size(), RTP_NO_FLAGS) != RTP_OK)
    {
        std::cerr << "Failed to send frame!" << std::endl;
        return;
    }

}
