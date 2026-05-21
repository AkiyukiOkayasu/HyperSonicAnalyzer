#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>

class HyperSonicAnalyzerProcessor : public juce::AudioProcessor
{
public:
    // FFTサイズ: 131072 サンプル (order = 17)
    static constexpr int fftOrder = 17;
    static constexpr int fftSize = 1 << fftOrder;   // 131072
    static constexpr int numBins = fftSize / 2 + 1; // 65537 bins

    HyperSonicAnalyzerProcessor();
    ~HyperSonicAnalyzerProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // スペクトラムデータへのアクセス
    const std::array<float, numBins>& getMagnitudeData() const { return magnitudeDataOutput; }
    bool isNewDataAvailable() const { return newDataAvailable.load(); }
    void resetNewDataFlag() { newDataAvailable.store(false); }

    double getCurrentSampleRate() const { return currentSampleRate.load(); }
    float getNyquistFrequency() const { return static_cast<float>(currentSampleRate.load() / 2.0); }

    // パラメータ
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // FFT処理（UIスレッドから呼び出す）
    void processFFT();

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // FFT
    juce::dsp::FFT fft;

    // Blackman-Harris 窓関数
    std::array<float, fftSize> windowFunction;
    void createBlackmanHarrisWindow();

    // FIFOバッファ（プッシュのみ、ロックフリー）
    std::array<float, fftSize> fifoBuffer;
    std::atomic<int> fifoWriteIndex{0};

    // FFT処理用バッファ（UIスレッドで処理）
    std::array<float, fftSize> fftInputBuffer;
    std::array<float, fftSize * 2> fftData;
    std::array<float, numBins> magnitudeDataOutput;

    // FFT準備完了フラグ
    std::atomic<bool> fftDataReady{false};
    std::atomic<bool> newDataAvailable{false};
    std::atomic<double> currentSampleRate{44100.0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HyperSonicAnalyzerProcessor)
};
