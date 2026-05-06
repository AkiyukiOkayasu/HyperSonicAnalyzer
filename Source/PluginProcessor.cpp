#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

HyperSonicAnalyzerProcessor::HyperSonicAnalyzerProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()),
      fft(fftOrder)
{
    fifoBuffer.fill(0.0f);
    fftInputBuffer.fill(0.0f);
    fftData.fill(0.0f);
    magnitudeDataOutput.fill(-192.0f);
    
    createBlackmanHarrisWindow();
}

HyperSonicAnalyzerProcessor::~HyperSonicAnalyzerProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout HyperSonicAnalyzerProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // 振幅範囲の最小値 (dB)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("minDb", 1),
        "Min dB",
        juce::NormalisableRange<float>(-192.0f, 0.0f, 1.0f),
        -120.0f
    ));
    
    // 振幅範囲の最大値 (dB)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("maxDb", 1),
        "Max dB",
        juce::NormalisableRange<float>(-192.0f, 20.0f, 1.0f),
        0.0f
    ));
    
    return { params.begin(), params.end() };
}

void HyperSonicAnalyzerProcessor::createBlackmanHarrisWindow()
{
    // 4-term Blackman-Harris window
    constexpr double a0 = 0.35875;
    constexpr double a1 = 0.48829;
    constexpr double a2 = 0.14128;
    constexpr double a3 = 0.01168;
    
    for (int i = 0; i < fftSize; ++i)
    {
        double x = static_cast<double>(i) / static_cast<double>(fftSize - 1);
        windowFunction[i] = static_cast<float>(
            a0 - a1 * std::cos(2.0 * juce::MathConstants<double>::pi * x)
               + a2 * std::cos(4.0 * juce::MathConstants<double>::pi * x)
               - a3 * std::cos(6.0 * juce::MathConstants<double>::pi * x)
        );
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
    
    // バッファをクリア
    fifoBuffer.fill(0.0f);
    fifoWriteIndex.store(0);
    fftDataReady.store(false);
    magnitudeDataOutput.fill(-192.0f);
}

void HyperSonicAnalyzerProcessor::releaseResources()
{
}

bool HyperSonicAnalyzerProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void HyperSonicAnalyzerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    
    juce::ScopedNoDenormals noDenormals;
    
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    // 未使用の出力チャンネルをクリア
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    
    // 入力サンプルをFIFOに書き込み（モノラルにミックス）
    // オーディオスレッドではFFT処理を行わない
    const int numSamples = buffer.getNumSamples();
    int writeIdx = fifoWriteIndex.load(std::memory_order_relaxed);
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float monoSample = 0.0f;
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            monoSample += buffer.getSample(channel, sample);
        }
        monoSample /= static_cast<float>(totalNumInputChannels);
        
        fifoBuffer[writeIdx] = monoSample;
        writeIdx = (writeIdx + 1) % fftSize;
    }
    
    fifoWriteIndex.store(writeIdx, std::memory_order_release);
    fftDataReady.store(true, std::memory_order_release);
}

void HyperSonicAnalyzerProcessor::processFFT()
{
    if (!fftDataReady.load(std::memory_order_acquire))
        return;
    
    // FIFOからデータをコピー（スナップショット取得）
    int readIdx = fifoWriteIndex.load(std::memory_order_acquire);
    
    for (int i = 0; i < fftSize; ++i)
    {
        int bufferIndex = (readIdx + i) % fftSize;
        fftInputBuffer[i] = fifoBuffer[bufferIndex];
    }
    
    // 窓関数を適用しながらFFTバッファにコピー
    for (int i = 0; i < fftSize; ++i)
    {
        fftData[i] = fftInputBuffer[i] * windowFunction[i];
    }
    
    // 虚部をゼロで初期化
    for (int i = fftSize; i < fftSize * 2; ++i)
    {
        fftData[i] = 0.0f;
    }
    
    // FFT実行
    fft.performRealOnlyForwardTransform(fftData.data(), true);
    
    // マグニチュードをdBで計算
    for (int i = 0; i < numBins; ++i)
    {
        float real = fftData[i * 2];
        float imag = fftData[i * 2 + 1];
        float magnitude = std::sqrt(real * real + imag * imag);
        
        // 正規化 (FFTサイズと窓関数のゲインで補正)
        magnitude /= static_cast<float>(fftSize);
        
        // dBに変換 (最小値を-192dBに制限)
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
    
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HyperSonicAnalyzerProcessor();
}
