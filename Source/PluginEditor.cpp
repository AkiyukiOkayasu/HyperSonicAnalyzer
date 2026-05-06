#include "PluginProcessor.h"
#include "PluginEditor.h"

HyperSonicAnalyzerEditor::HyperSonicAnalyzerEditor(HyperSonicAnalyzerProcessor& p)
    : AudioProcessorEditor(&p), 
      audioProcessor(p),
      spectralAnalyzer(p)
{
    // タイトル
    titleLabel.setText("HyperSonic Analyzer", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // スペクトラムアナライザー
    addAndMakeVisible(spectralAnalyzer);
    
    // Min dB ダイヤル
    minDbSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    minDbSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    minDbSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0, 150, 255));
    minDbSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    minDbSlider.setRange(-192.0, 0.0, 1.0);
    minDbSlider.setTextValueSuffix(" dB");
    minDbSlider.onValueChange = [this]() {
        spectralAnalyzer.setMinDb(static_cast<float>(minDbSlider.getValue()));
    };
    addAndMakeVisible(minDbSlider);
    
    minDbLabel.setText("Min Level", juce::dontSendNotification);
    minDbLabel.setFont(juce::Font(12.0f));
    minDbLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    minDbLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(minDbLabel);
    
    minDbAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "minDb", minDbSlider);
    
    // Max dB ダイヤル
    maxDbSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    maxDbSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    maxDbSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(255, 150, 0));
    maxDbSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    maxDbSlider.setRange(-192.0, 20.0, 1.0);
    maxDbSlider.setTextValueSuffix(" dB");
    maxDbSlider.onValueChange = [this]() {
        spectralAnalyzer.setMaxDb(static_cast<float>(maxDbSlider.getValue()));
    };
    addAndMakeVisible(maxDbSlider);
    
    maxDbLabel.setText("Max Level", juce::dontSendNotification);
    maxDbLabel.setFont(juce::Font(12.0f));
    maxDbLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    maxDbLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(maxDbLabel);
    
    maxDbAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "maxDb", maxDbSlider);
    
    // 初期値を反映
    spectralAnalyzer.setMinDb(static_cast<float>(minDbSlider.getValue()));
    spectralAnalyzer.setMaxDb(static_cast<float>(maxDbSlider.getValue()));
    
    // ウィンドウサイズ
    setSize(1000, 600);
    setResizable(true, true);
    setResizeLimits(800, 500, 2000, 1200);
}

HyperSonicAnalyzerEditor::~HyperSonicAnalyzerEditor()
{
}

void HyperSonicAnalyzerEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(25, 25, 35));
}

void HyperSonicAnalyzerEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // タイトル領域
    auto titleArea = bounds.removeFromTop(40);
    titleLabel.setBounds(titleArea);
    
    // コントロール領域（下部）
    auto controlArea = bounds.removeFromBottom(120);
    
    // スペクトラムアナライザー領域
    spectralAnalyzer.setBounds(bounds.reduced(10));
    
    // コントロールを配置
    auto controlBounds = controlArea.reduced(20, 10);
    int dialWidth = 100;
    int dialHeight = 90;
    int spacing = 30;
    
    auto minDbArea = controlBounds.removeFromLeft(dialWidth);
    minDbLabel.setBounds(minDbArea.removeFromTop(20));
    minDbSlider.setBounds(minDbArea.removeFromTop(dialHeight));
    
    controlBounds.removeFromLeft(spacing);
    
    auto maxDbArea = controlBounds.removeFromLeft(dialWidth);
    maxDbLabel.setBounds(maxDbArea.removeFromTop(20));
    maxDbSlider.setBounds(maxDbArea.removeFromTop(dialHeight));
}
