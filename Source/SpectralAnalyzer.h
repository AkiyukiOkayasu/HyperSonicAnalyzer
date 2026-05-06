#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

// Forward declaration
class HyperSonicAnalyzerProcessor;

class SpectralAnalyzer : public juce::Component,
                          public juce::Timer
{
public:
    static constexpr int numBins = 32769; // fftSize / 2 + 1

    SpectralAnalyzer(HyperSonicAnalyzerProcessor& processor);
    ~SpectralAnalyzer() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void setMinDb(float minDb) { minDbValue = minDb; }
    void setMaxDb(float maxDb) { maxDbValue = maxDb; }

private:
    HyperSonicAnalyzerProcessor& audioProcessor;
    
    std::array<float, numBins> displayData;
    
    float minDbValue = -120.0f;
    float maxDbValue = 0.0f;
    
    // スムージング係数
    static constexpr float smoothingCoeff = 0.7f;
    
    // 周波数をX座標に変換（対数スケール）
    float frequencyToX(float frequency, float width, float minFreq, float maxFreq) const;
    
    // dB値をY座標に変換
    float dbToY(float db, float height) const;
    
    // グリッドとラベルを描画
    void drawGrid(juce::Graphics& g, float width, float height, float nyquist);
    void drawFrequencyLabels(juce::Graphics& g, float width, float height, float nyquist);
    void drawDbLabels(juce::Graphics& g, float width, float height);
    
    // 周波数文字列のフォーマット
    juce::String formatFrequency(float freq) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralAnalyzer)
};
