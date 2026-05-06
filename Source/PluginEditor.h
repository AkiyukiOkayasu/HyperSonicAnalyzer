#pragma once

#include "PluginProcessor.h"
#include "SpectralAnalyzer.h"

class HyperSonicAnalyzerEditor : public juce::AudioProcessorEditor
{
public:
    explicit HyperSonicAnalyzerEditor(HyperSonicAnalyzerProcessor&);
    ~HyperSonicAnalyzerEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    HyperSonicAnalyzerProcessor& audioProcessor;
    
    SpectralAnalyzer spectralAnalyzer;
    
    // 振幅範囲設定用ダイヤル
    juce::Slider minDbSlider;
    juce::Slider maxDbSlider;
    
    juce::Label minDbLabel;
    juce::Label maxDbLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> minDbAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> maxDbAttachment;
    
    // スケール切替ボタン
    juce::ToggleButton linearScaleButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> linearScaleAttachment;
    
    // タイトルラベル
    juce::Label titleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HyperSonicAnalyzerEditor)
};
