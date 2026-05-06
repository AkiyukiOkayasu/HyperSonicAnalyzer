#include "SpectralAnalyzer.h"
#include "PluginProcessor.h"
#include <cmath>

SpectralAnalyzer::SpectralAnalyzer(HyperSonicAnalyzerProcessor& processor)
    : audioProcessor(processor)
{
    displayData.fill(-192.0f);
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
    startTimerHz(60); // 60fps更新
}

SpectralAnalyzer::~SpectralAnalyzer()
{
    stopTimer();
}

void SpectralAnalyzer::timerCallback()
{
    // UIスレッドでFFT処理を実行
    audioProcessor.processFFT();
    
    if (audioProcessor.isNewDataAvailable())
    {
        const auto& newData = audioProcessor.getMagnitudeData();
        
        // スムージングを適用
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

float SpectralAnalyzer::frequencyToX(float frequency, float width, float minFreq, float maxFreq) const
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
    else if (freq >= 1000.0f)
        return juce::String(freq / 1000.0f, 2) + " kHz";
    else
        return juce::String(freq, 1) + " Hz";
}

void SpectralAnalyzer::drawGrid(juce::Graphics& g, float width, float height, float nyquist)
{
    g.setColour(juce::Colour(60, 60, 70));
    
    float minFreq = 10.0f;
    float maxFreq = nyquist;
    
    // 対数スケール: 対数間隔のグリッド
    std::vector<float> gridFreqs = { 10, 20, 50, 100, 200, 500, 
                                      1000, 2000, 5000, 10000, 20000, 
                                      50000, 100000, 200000, 384000 };
    
    for (float freq : gridFreqs)
    {
        if (freq > nyquist) break;
        
        float x = frequencyToX(freq, width, minFreq, maxFreq);
        g.drawVerticalLine(static_cast<int>(x), 0.0f, height);
    }
    
    // dBグリッド線
    float dbRange = maxDbValue - minDbValue;
    float dbStep = 10.0f;
    if (dbRange > 100) dbStep = 20.0f;
    if (dbRange > 150) dbStep = 30.0f;
    
    for (float db = minDbValue; db <= maxDbValue; db += dbStep)
    {
        float y = dbToY(db, height);
        g.drawHorizontalLine(static_cast<int>(y), 0.0f, width);
    }
}

void SpectralAnalyzer::drawFrequencyLabels(juce::Graphics& g, float width, float height, float nyquist)
{
    g.setColour(juce::Colours::lightgrey);
    g.setFont(11.0f);
    
    float minFreq = 10.0f;
    float maxFreq = nyquist;
    
    std::vector<float> labelFreqs = { 20, 100, 1000, 10000, 20000, 100000, 200000 };
    
    for (float freq : labelFreqs)
    {
        if (freq > nyquist) break;
        
        float x = frequencyToX(freq, width, minFreq, maxFreq);
        juce::String label = formatFrequency(freq);
        
        g.drawText(label, 
                   static_cast<int>(x) - 25, static_cast<int>(height) - 18, 
                   50, 16, juce::Justification::centred);
    }
    
    // ナイキスト周波数を常に表示
    float x = frequencyToX(nyquist, width, minFreq, maxFreq);
    juce::String nyquistLabel = formatFrequency(nyquist) + " (Nyquist)";
    g.setColour(juce::Colours::cyan);
    g.drawText(nyquistLabel, 
               static_cast<int>(x) - 50, static_cast<int>(height) - 18, 
               100, 16, juce::Justification::centred);
}

void SpectralAnalyzer::drawDbLabels(juce::Graphics& g, float width, float height)
{
    juce::ignoreUnused(width);
    
    g.setColour(juce::Colours::lightgrey);
    g.setFont(10.0f);
    
    float dbRange = maxDbValue - minDbValue;
    float dbStep = 10.0f;
    if (dbRange > 100) dbStep = 20.0f;
    if (dbRange > 150) dbStep = 30.0f;
    
    for (float db = minDbValue; db <= maxDbValue; db += dbStep)
    {
        float y = dbToY(db, height);
        juce::String label = juce::String(static_cast<int>(db)) + " dB";
        
        g.drawText(label, 2, static_cast<int>(y) - 8, 50, 16, juce::Justification::left);
    }
}

void SpectralAnalyzer::drawCrosshair(juce::Graphics& g, float width, float height, float nyquist)
{
    if (!mouseOver)
        return;
    
    float minFreq = 10.0f;
    float maxFreq = nyquist;
    float sampleRate = static_cast<float>(audioProcessor.getCurrentSampleRate());
    
    // マウス位置から周波数を計算
    float frequency = xToFrequency(mousePos.x, width, minFreq, maxFreq);
    frequency = std::clamp(frequency, minFreq, maxFreq);
    
    // 対応するビンの振幅を取得
    int bin = frequencyToBin(frequency, sampleRate);
    float amplitude = displayData[bin];
    
    // クロスヘアを描画
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawVerticalLine(static_cast<int>(mousePos.x), 0.0f, height);
    
    float yPos = dbToY(amplitude, height);
    g.drawHorizontalLine(static_cast<int>(yPos), 0.0f, width);
    
    // 交点にマーカー
    g.setColour(juce::Colours::white);
    g.fillEllipse(mousePos.x - 4, yPos - 4, 8, 8);
    
    // 情報ボックスを描画
    juce::String freqText = formatFrequency(frequency);
    juce::String ampText = juce::String(amplitude, 1) + " dB";
    juce::String infoText = freqText + " / " + ampText;
    
    g.setFont(13.0f);
    float textWidth = g.getCurrentFont().getStringWidth(infoText) + 16;
    float textHeight = 22;
    
    // ボックスの位置を調整（画面端での切れを防ぐ）
    float boxX = mousePos.x + 15;
    float boxY = mousePos.y - 30;
    
    if (boxX + textWidth > width)
        boxX = mousePos.x - textWidth - 15;
    if (boxY < 0)
        boxY = mousePos.y + 15;
    
    // 背景ボックス
    g.setColour(juce::Colour(40, 40, 50).withAlpha(0.9f));
    g.fillRoundedRectangle(boxX, boxY, textWidth, textHeight, 4.0f);
    
    g.setColour(juce::Colours::cyan);
    g.drawRoundedRectangle(boxX, boxY, textWidth, textHeight, 4.0f, 1.0f);
    
    // テキスト
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
    
    // 背景
    g.fillAll(juce::Colour(20, 20, 30));
    
    float nyquist = audioProcessor.getNyquistFrequency();
    float sampleRate = static_cast<float>(audioProcessor.getCurrentSampleRate());
    
    // グリッドを描画
    drawGrid(g, width, height, nyquist);
    
    // スペクトラムを描画
    juce::Path spectrumPath;
    
    float minFreq = 10.0f;
    float maxFreq = nyquist;
    float binWidth = sampleRate / static_cast<float>(HyperSonicAnalyzerProcessor::fftSize);
    
    bool pathStarted = false;
    
    for (int i = 1; i < numBins; ++i)
    {
        float frequency = static_cast<float>(i) * binWidth;
        
        if (frequency < minFreq)
            continue;
        if (frequency > maxFreq)
            break;
        
        float x = frequencyToX(frequency, width, minFreq, maxFreq);
        float y = dbToY(displayData[i], height);
        
        if (!pathStarted)
        {
            spectrumPath.startNewSubPath(x, y);
            pathStarted = true;
        }
        else
        {
            spectrumPath.lineTo(x, y);
        }
    }
    
    // スペクトラムの塗りつぶし
    juce::Path fillPath = spectrumPath;
    if (pathStarted)
    {
        fillPath.lineTo(width, height);
        fillPath.lineTo(0, height);
        fillPath.closeSubPath();
        
        g.setGradientFill(juce::ColourGradient(
            juce::Colour(0, 150, 255).withAlpha(0.5f), 0, 0,
            juce::Colour(0, 50, 150).withAlpha(0.2f), 0, height,
            false
        ));
        g.fillPath(fillPath);
    }
    
    // スペクトラムライン
    g.setColour(juce::Colour(0, 200, 255));
    g.strokePath(spectrumPath, juce::PathStrokeType(0.75f));
    
    // クロスヘアを描画
    drawCrosshair(g, width, height, nyquist);
    
    // ラベルを描画
    drawFrequencyLabels(g, width, height, nyquist);
    drawDbLabels(g, width, height);
    
    // サンプルレート表示
    g.setColour(juce::Colours::yellow);
    g.setFont(12.0f);
    juce::String srText = "Sample Rate: " + formatFrequency(sampleRate);
    g.drawText(srText, static_cast<int>(width) - 180, 5, 175, 20, juce::Justification::right);
}

void SpectralAnalyzer::resized()
{
}
