#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <cmath>

HyperSonicAnalyzerProcessor::HyperSonicAnalyzerProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()), fft(fftOrder)
{
    fifoBuffer.fill(0.0f);
    fftInputBuffer.fill(0.0f);
    fftData.fill(0.0f);
    magnitudeDataOutput.fill(-192.0f);

    createBlackmanHarrisWindow();
}

HyperSonicAnalyzerProcessor::~HyperSonicAnalyzerProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout
HyperSonicAnalyzerProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("minDb", 1), "Min dB",
        juce::NormalisableRange<float>(-192.0f, 0.0f, 1.0f), -120.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("maxDb", 1), "Max dB",
        juce::NormalisableRange<float>(-192.0f, 20.0f, 1.0f), 0.0f));

    return {params.begin(), params.end()};
}

void HyperSonicAnalyzerProcessor::createBlackmanHarrisWindow()
{
    constexpr double a0 = 0.35875;
    constexpr double a1 = 0.48829;
    constexpr double a2 = 0.14128;
    constexpr double a3 = 0.01168;

    for (size_t i = 0; i < static_cast<size_t>(fftSize); ++i)
    {
        double x = static_cast<double>(i) / static_cast<double>(fftSize - 1);
        windowFunction[i] =
            static_cast<float>(a0 - a1 * std::cos(2.0 * juce::MathConstants<double>::pi * x) +
                               a2 * std::cos(4.0 * juce::MathConstants<double>::pi * x) -
                               a3 * std::cos(6.0 * juce::MathConstants<double>::pi * x));
    }
}

const juce::String HyperSonicAnalyzerProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HyperSonicAnalyzerProcessor::acceptsMidi() const
{
    return false;
}
bool HyperSonicAnalyzerProcessor::producesMidi() const
{
    return false;
}
bool HyperSonicAnalyzerProcessor::isMidiEffect() const
{
    return false;
}
double HyperSonicAnalyzerProcessor::getTailLengthSeconds() const
{
    return 0.0;
}
int HyperSonicAnalyzerProcessor::getNumPrograms()
{
    return 1;
}
int HyperSonicAnalyzerProcessor::getCurrentProgram()
{
    return 0;
}

void HyperSonicAnalyzerProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String HyperSonicAnalyzerProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void HyperSonicAnalyzerProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void HyperSonicAnalyzerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);

    currentSampleRate.store(sampleRate);

    fifoBuffer.fill(0.0f);
    fifoWriteIndex.store(0);
    fftDataReady.store(false);
    magnitudeDataOutput.fill(-192.0f);
}

void HyperSonicAnalyzerProcessor::releaseResources() {}

bool HyperSonicAnalyzerProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void HyperSonicAnalyzerProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();
    auto writeIdx = static_cast<size_t>(fifoWriteIndex.load(std::memory_order_relaxed));

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float monoSample = 0.0f;
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            monoSample += buffer.getSample(channel, sample);
        }
        monoSample /= static_cast<float>(totalNumInputChannels);

        fifoBuffer[writeIdx] = monoSample;
        writeIdx = (writeIdx + 1) % static_cast<size_t>(fftSize);
    }

    fifoWriteIndex.store(static_cast<int>(writeIdx), std::memory_order_release);
    fftDataReady.store(true, std::memory_order_release);
}

void HyperSonicAnalyzerProcessor::processFFT()
{
    if (!fftDataReady.load(std::memory_order_acquire))
        return;

    auto readIdx = static_cast<size_t>(fifoWriteIndex.load(std::memory_order_acquire));

    for (size_t i = 0; i < static_cast<size_t>(fftSize); ++i)
    {
        size_t bufferIndex = (readIdx + i) % static_cast<size_t>(fftSize);
        fftInputBuffer[i] = fifoBuffer[bufferIndex];
    }

    for (size_t i = 0; i < static_cast<size_t>(fftSize); ++i)
    {
        fftData[i] = fftInputBuffer[i] * windowFunction[i];
    }

    for (size_t i = static_cast<size_t>(fftSize); i < static_cast<size_t>(fftSize * 2); ++i)
    {
        fftData[i] = 0.0f;
    }

    fft.performRealOnlyForwardTransform(fftData.data(), true);

    for (size_t i = 0; i < static_cast<size_t>(numBins); ++i)
    {
        float real = fftData[i * 2];
        float imag = fftData[i * 2 + 1];
        float magnitude = std::sqrt(real * real + imag * imag);

        magnitude /= static_cast<float>(fftSize);

        if (magnitude > 0.0f)
        {
            magnitudeDataOutput[i] = 20.0f * std::log10(magnitude);
            magnitudeDataOutput[i] = std::max(magnitudeDataOutput[i], -192.0f);
        }
        else
        {
            magnitudeDataOutput[i] = -192.0f;
        }
    }

    newDataAvailable.store(true);
}

bool HyperSonicAnalyzerProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* HyperSonicAnalyzerProcessor::createEditor()
{
    return new HyperSonicAnalyzerEditor(*this);
}

void HyperSonicAnalyzerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HyperSonicAnalyzerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HyperSonicAnalyzerProcessor();
}
