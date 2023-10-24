#include "PluginProcessor.h"
#include "PluginEditor.h"

constexpr int TEST_PACKETS      = 100;
constexpr int PAYLOAD_MAXLEN    = 0xffff - 0x1000;

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

    //Start frequency by default
    toneGenerator.setFrequency(440.0f);
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
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto totalNumberOfSamples = buffer.getNumSamples();

    //Tone Generator
    channelInfo.buffer = &buffer;
    channelInfo.numSamples = totalNumberOfSamples;
    toneGenerator.getNextAudioBlock(channelInfo);

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelDataIn = buffer.getReadPointer(channel);
        auto* channelDataOut = buffer.getWritePointer(channel);
        for (int sample = 0; sample < totalNumberOfSamples; ++sample)
        {
            channelDataOut[sample] = buffer.getReadPointer(channel)[sample];
        }
    }
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
void AudioStreamPluginProcessor::streamIn(int localPort)
{
    if (!streamSessionID)
    {
        std::cout << "A session has not been created" << std::endl;
        return;
    }
    streamID = pRTP->CreateStream(streamSessionID, localPort, 1);
    if (!streamID)
    {
        std::cout << "Failed to create stream" << std::endl;
        return;
    }
    auto pStream = UVGRTPWrap::GetSP(pRTP)->GetStream(streamID);
    if (!pStream)
    {
        std::cout << "Failed to get stream pointer." << std::endl;
    }
    if (pStream->install_receive_hook(nullptr,
            +[](void*, uvgrtp::frame::rtp_frame* pFrame) -> void {
                std::cout << "Received RTP pFrame. Payload size: " << pFrame->payload_len << std::endl;
                uvgrtp::frame::dealloc_frame(pFrame);
    }) != RTP_OK)
    {
        std::cout << "Failed to install RTP receive hook!" << std::endl;
        return;
    }
}
void AudioStreamPluginProcessor::streamOut(int remotePort, float* fVector, int numSamples)
{
    //Create a stream
    if (!streamID)
    {
        streamID = pRTP->CreateStream(streamSessionID, remotePort, 0);
    }
    if (!streamID)
    {
        std::cout << "Failed to create stream" << std::endl;
        return;
    }
    auto media = std::unique_ptr<uint8_t []>(new uint8_t[PAYLOAD_MAXLEN]);
    srand(static_cast<unsigned int>(time(nullptr)));
    auto pStream = UVGRTPWrap::GetSP(pRTP)->GetStream(streamID);

}
void AudioStreamPluginProcessor::streamOut (int remotePort)
{
    //Ok create a stream
    if (!streamSessionID)
    {
        std::cout << "A session has not been created" << std::endl;
        return;
    }
    streamID = pRTP->CreateStream(streamSessionID, remotePort, 0);
    if (!streamID)
    {
        std::cout << "Failed to create stream" << std::endl;
        return;
    }
    auto media = std::unique_ptr<uint8_t []>(new uint8_t[PAYLOAD_MAXLEN]);
    srand(static_cast<unsigned int>(time(nullptr)));
    //0 unsigned int literal
    //Retrieve UVGRTP driver.
    auto pStream = UVGRTPWrap::GetSP(pRTP)->GetStream(streamID);
    for (int i = 0; i < TEST_PACKETS; ++i)
    {
        size_t random_packet_size = static_cast<size_t>(rand() % PAYLOAD_MAXLEN + 1);
        for (size_t j = 0; j < random_packet_size; ++j)
        {
            media[j] = (j + random_packet_size) % CHAR_MAX;
        }
        std::cout << "Sending RTP frame " << i + 1 << '/' << TEST_PACKETS << ". Payload size: " << random_packet_size << std::endl;
        if (pStream->push_frame(media.get(), random_packet_size, RTP_NO_FLAGS) != RTP_OK)
        {
            std::cerr << "Failed to send frame!" << std::endl;
            return;
        }
    }

}