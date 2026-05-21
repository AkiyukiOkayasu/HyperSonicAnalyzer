#include "PluginEditor.h"

#include "PluginProcessor.h"

HyperSonicAnalyzerEditor::HyperSonicAnalyzerEditor(HyperSonicAnalyzerProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), spectralAnalyzer(p)
{
    addAndMakeVisible(spectralAnalyzer);

    minDbSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    minDbSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    minDbSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::mediumspringgreen);
    minDbSlider.setColour(juce::Slider::thumbColourId, juce::Colours::lightgrey);
    minDbSlider.setRange(-192.0, 0.0, 1.0);
    minDbSlider.setTextValueSuffix(" dB");
    minDbSlider.onValueChange = [this]()
    {
        spectralAnalyzer.setMinDb(static_cast<float>(minDbSlider.getValue()));
    };
    addAndMakeVisible(minDbSlider);

    minDbLabel.setText("Min Level", juce::dontSendNotification);
    minDbLabel.setFont(juce::FontOptions(12.0f));
    minDbLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    minDbLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(minDbLabel);

    minDbAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "minDb", minDbSlider);

    maxDbSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    maxDbSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    maxDbSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::mediumspringgreen);
    maxDbSlider.setColour(juce::Slider::thumbColourId, juce::Colours::lightgrey);
    maxDbSlider.setRange(-192.0, 20.0, 1.0);
    maxDbSlider.setTextValueSuffix(" dB");
    maxDbSlider.onValueChange = [this]()
    {
        spectralAnalyzer.setMaxDb(static_cast<float>(maxDbSlider.getValue()));
    };
    addAndMakeVisible(maxDbSlider);

    maxDbLabel.setText("Max Level", juce::dontSendNotification);
    maxDbLabel.setFont(juce::FontOptions(12.0f));
    maxDbLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    maxDbLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(maxDbLabel);

    maxDbAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "maxDb", maxDbSlider);

    spectralAnalyzer.setMinDb(static_cast<float>(minDbSlider.getValue()));
    spectralAnalyzer.setMaxDb(static_cast<float>(maxDbSlider.getValue()));

    setSize(1000, 600);
    setResizable(true, true);
    setResizeLimits(800, 500, 4096, 2160);
}

HyperSonicAnalyzerEditor::~HyperSonicAnalyzerEditor() = default;

void HyperSonicAnalyzerEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(25, 25, 35));
}

void HyperSonicAnalyzerEditor::resized()
{
    auto bounds = getLocalBounds();

    auto controlArea = bounds.removeFromBottom(120);

    spectralAnalyzer.setBounds(bounds.reduced(10));

    auto controlBounds = controlArea.reduced(20, 10);
    constexpr int dialWidth = 100;
    constexpr int dialHeight = 90;
    constexpr int spacing = 30;

    auto minDbArea = controlBounds.removeFromLeft(dialWidth);
    minDbLabel.setBounds(minDbArea.removeFromTop(20));
    minDbSlider.setBounds(minDbArea.removeFromTop(dialHeight));

    controlBounds.removeFromLeft(spacing);

    auto maxDbArea = controlBounds.removeFromLeft(dialWidth);
    maxDbLabel.setBounds(maxDbArea.removeFromTop(20));
    maxDbSlider.setBounds(maxDbArea.removeFromTop(dialHeight));
}
