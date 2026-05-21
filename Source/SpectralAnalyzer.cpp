#include "SpectralAnalyzer.h"

#include "PluginProcessor.h"

#include <cmath>

SpectralAnalyzer::SpectralAnalyzer(HyperSonicAnalyzerProcessor& processor)
    : audioProcessor(processor)
{
    displayData.fill(-192.0f);
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
    startTimerHz(60);
}

SpectralAnalyzer::~SpectralAnalyzer()
{
    stopTimer();
}

void SpectralAnalyzer::timerCallback()
{
    audioProcessor.processFFT();

    if (audioProcessor.isNewDataAvailable())
    {
        const auto& newData = audioProcessor.getMagnitudeData();

        for (size_t i = 0; i < displayData.size(); ++i)
        {
            displayData[i] = displayData[i] * smoothingCoeff + newData[i] * (1.0f - smoothingCoeff);
        }

        audioProcessor.resetNewDataFlag();
        repaint();
    }
}

void SpectralAnalyzer::mouseMove(const juce::MouseEvent& event)
{
    mousePos = event.position;
    repaint();
}

void SpectralAnalyzer::mouseEnter(const juce::MouseEvent& event)
{
    mouseOver = true;
    mousePos = event.position;
    repaint();
}

void SpectralAnalyzer::mouseExit(const juce::MouseEvent&)
{
    mouseOver = false;
    repaint();
}

float SpectralAnalyzer::frequencyToX(float frequency, float width, float minFreq,
                                     float maxFreq) const
{
    if (frequency <= 0.0f || minFreq <= 0.0f)
        return 0.0f;

    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);
    float logFreq = std::log10(frequency);

    return width * (logFreq - logMin) / (logMax - logMin);
}

float SpectralAnalyzer::xToFrequency(float x, float width, float minFreq, float maxFreq) const
{
    if (width <= 0.0f)
        return minFreq;

    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);
    float logFreq = logMin + (x / width) * (logMax - logMin);

    return std::pow(10.0f, logFreq);
}

int SpectralAnalyzer::frequencyToBin(float frequency, float sampleRate) const
{
    float binWidth = sampleRate / static_cast<float>(HyperSonicAnalyzerProcessor::fftSize);
    int bin = static_cast<int>(std::round(frequency / binWidth));
    return std::clamp(bin, 0, numBins - 1);
}

float SpectralAnalyzer::dbToY(float db, float height) const
{
    float normalizedDb = (db - minDbValue) / (maxDbValue - minDbValue);
    normalizedDb = std::clamp(normalizedDb, 0.0f, 1.0f);
    return height * (1.0f - normalizedDb);
}

juce::String SpectralAnalyzer::formatFrequency(float freq) const
{
    if (freq >= 1000000.0f)
        return juce::String(freq / 1000000.0f, 2) + " MHz";
    if (freq >= 1000.0f)
        return juce::String(freq / 1000.0f, 2) + " kHz";
    return juce::String(freq, 1) + " Hz";
}

juce::String SpectralAnalyzer::formatFrequencyShort(float freq) const
{
    if (freq >= 1000000.0f)
    {
        float val = freq / 1000000.0f;
        return (val == std::floor(val)) ? juce::String(static_cast<int>(val)) + "M"
                                        : juce::String(val, 1) + "M";
    }
    if (freq >= 1000.0f)
    {
        float val = freq / 1000.0f;
        return (val == std::floor(val)) ? juce::String(static_cast<int>(val)) + "k"
                                        : juce::String(val, 1) + "k";
    }
    return juce::String(static_cast<int>(freq));
}

void SpectralAnalyzer::drawGrid(juce::Graphics& g, float width, float height, float nyquist)
{
    constexpr float minFreq = 10.0f;
    const float maxFreq = nyquist;

    const std::array<float, 7> labelFreqs = {20, 100, 1000, 10000, 20000, 100000, 200000};

    const std::array<float, 36> subFreqs = {
        10,   30,   40,   50,   60,   70,   80,   90,   200,   300,  400,  500,
        600,  700,  800,  900,  2000,  3000, 4000, 5000, 6000,  7000, 8000, 9000,
        20000,30000,40000,50000,60000, 70000,80000,90000,300000,384000};

    g.setColour(juce::Colour(35, 35, 45));
    for (float freq : subFreqs)
    {
        if (freq > nyquist || freq < minFreq)
            continue;
        float x = frequencyToX(freq, width, minFreq, maxFreq);
        g.drawVerticalLine(static_cast<int>(x), 0.0f, height);
    }

    g.setColour(juce::Colour(60, 60, 70));
    for (float freq : labelFreqs)
    {
        if (freq > nyquist)
            break;
        float x = frequencyToX(freq, width, minFreq, maxFreq);
        g.drawVerticalLine(static_cast<int>(x), 0.0f, height);
    }

    g.setColour(juce::Colour(35, 35, 45));
    for (float db = 0.0f; db >= minDbValue; db -= 6.0f)
    {
        float y = dbToY(db, height);
        g.drawHorizontalLine(static_cast<int>(y), 0.0f, width);
    }
}

void SpectralAnalyzer::drawFrequencyLabels(juce::Graphics& g, float width, float height,
                                           float nyquist)
{
    g.setColour(juce::Colours::lightgrey);
    g.setFont(11.0f);

    constexpr float minFreq = 10.0f;
    const float maxFreq = nyquist;

    const std::array<float, 7> labelFreqs = {20, 100, 1000, 10000, 20000, 100000, 200000};

    for (float freq : labelFreqs)
    {
        if (freq > nyquist)
            break;

        float x = frequencyToX(freq, width, minFreq, maxFreq);
        juce::String label = formatFrequencyShort(freq);

        g.drawText(label, static_cast<int>(x) - 25, static_cast<int>(height) - 18, 50, 16,
                   juce::Justification::centred);
    }
}

void SpectralAnalyzer::drawDbLabels(juce::Graphics& g, float /*width*/, float height)
{
    g.setColour(juce::Colours::lightgrey);
    g.setFont(10.0f);

    for (float db = 0.0f; db >= minDbValue; db -= 6.0f)
    {
        float y = dbToY(db, height);
        g.drawText(juce::String(static_cast<int>(db)), 2, static_cast<int>(y) - 8, 30, 16,
                   juce::Justification::left);
    }
}

void SpectralAnalyzer::drawCrosshair(juce::Graphics& g, float width, float height, float nyquist)
{
    if (!mouseOver)
        return;

    constexpr float minFreq = 10.0f;
    const float maxFreq = nyquist;
    float sampleRate = static_cast<float>(audioProcessor.getCurrentSampleRate());

    float frequency = xToFrequency(mousePos.x, width, minFreq, maxFreq);
    frequency = std::clamp(frequency, minFreq, maxFreq);

    int bin = frequencyToBin(frequency, sampleRate);
    float amplitude = displayData[static_cast<size_t>(bin)];

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawVerticalLine(static_cast<int>(mousePos.x), 0.0f, height);

    float yPos = dbToY(amplitude, height);
    g.drawHorizontalLine(static_cast<int>(yPos), 0.0f, width);

    g.setColour(juce::Colours::white);
    g.fillEllipse(mousePos.x - 4.0f, yPos - 4.0f, 8.0f, 8.0f);

    juce::String freqText = formatFrequency(frequency);
    juce::String ampText = juce::String(amplitude, 1) + " dB";
    juce::String infoText = freqText + " / " + ampText;

    g.setFont(13.0f);
    auto textWidth = static_cast<float>(infoText.length()) * 8.0f + 16.0f;
    constexpr float textHeight = 22.0f;

    float boxX = mousePos.x + 15.0f;
    float boxY = mousePos.y - 30.0f;

    if (boxX + textWidth > width)
        boxX = mousePos.x - textWidth - 15.0f;
    if (boxY < 0.0f)
        boxY = mousePos.y + 15.0f;

    g.setColour(juce::Colour(40, 40, 50).withAlpha(0.9f));
    g.fillRoundedRectangle(boxX, boxY, textWidth, textHeight, 4.0f);

    g.setColour(juce::Colours::white);
    g.drawText(infoText, static_cast<int>(boxX), static_cast<int>(boxY),
               static_cast<int>(textWidth), static_cast<int>(textHeight),
               juce::Justification::centred);
}

void SpectralAnalyzer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float width = bounds.getWidth();
    float height = bounds.getHeight();

    g.fillAll(juce::Colour(20, 20, 30));

    float nyquist = audioProcessor.getNyquistFrequency();
    float sampleRate = static_cast<float>(audioProcessor.getCurrentSampleRate());

    drawGrid(g, width, height, nyquist);

    juce::Path spectrumPath;

    constexpr float minFreq = 10.0f;
    const float maxFreq = nyquist;
    float binWidth = sampleRate / static_cast<float>(HyperSonicAnalyzerProcessor::fftSize);

    bool pathStarted = false;

    // ピクセルごとに最大値を描画（高域の密集を軽減）
    int lastPixelX = -1;
    float maxDbForPixel = -std::numeric_limits<float>::infinity();
    float pixelX = 0.0f;

    for (size_t i = 1; i < static_cast<size_t>(numBins); ++i)
    {
        float frequency = static_cast<float>(i) * binWidth;

        if (frequency < minFreq)
            continue;
        if (frequency > maxFreq)
            break;

        float x = frequencyToX(frequency, width, minFreq, maxFreq);
        int currentPixelX = static_cast<int>(x);
        float db = displayData[i];

        if (currentPixelX == lastPixelX)
        {
            // 同じピクセル内では最大値を追跡
            if (db > maxDbForPixel)
            {
                maxDbForPixel = db;
            }
        }
        else
        {
            // 前のピクセルの最大値を描画
            if (lastPixelX >= 0)
            {
                float y = dbToY(maxDbForPixel, height);
                if (!pathStarted)
                {
                    spectrumPath.startNewSubPath(pixelX, y);
                    pathStarted = true;
                }
                else
                {
                    spectrumPath.lineTo(pixelX, y);
                }
            }

            // 新しいピクセルの追跡を開始
            lastPixelX = currentPixelX;
            pixelX = x;
            maxDbForPixel = db;
        }
    }

    // 最後のピクセルを描画
    if (lastPixelX >= 0)
    {
        float y = dbToY(maxDbForPixel, height);
        if (!pathStarted)
        {
            spectrumPath.startNewSubPath(pixelX, y);
            pathStarted = true;
        }
        else
        {
            spectrumPath.lineTo(pixelX, y);
        }
    }

    juce::Path fillPath = spectrumPath;
    if (pathStarted)
    {
        fillPath.lineTo(width, height);
        fillPath.lineTo(0.0f, height);
        fillPath.closeSubPath();

        g.setGradientFill(juce::ColourGradient(juce::Colour(0, 150, 255).withAlpha(0.5f), 0.0f,
                                               0.0f, juce::Colour(0, 50, 150).withAlpha(0.2f), 0.0f,
                                               height, false));
        g.fillPath(fillPath);
    }

    g.setColour(juce::Colour(0, 200, 255));
    g.strokePath(spectrumPath, juce::PathStrokeType(0.5f));

    drawCrosshair(g, width, height, nyquist);

    drawFrequencyLabels(g, width, height, nyquist);
    drawDbLabels(g, width, height);

    g.setColour(juce::Colours::yellow);
    g.setFont(12.0f);
    juce::String srText = "Sample Rate: " + formatFrequency(sampleRate);
    g.drawText(srText, static_cast<int>(width) - 180, 5, 175, 20, juce::Justification::right);
}

void SpectralAnalyzer::resized() {}
