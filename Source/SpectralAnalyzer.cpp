#include "SpectralAnalyzer.h"
#include "PluginProcessor.h"
#include <cmath>

SpectralAnalyzer::SpectralAnalyzer(HyperSonicAnalyzerProcessor& processor)
    : audioProcessor(processor)
{
    displayData.fill(-192.0f);
    startTimerHz(60); // 60fps更新
}

SpectralAnalyzer::~SpectralAnalyzer()
{
    stopTimer();
}

void SpectralAnalyzer::timerCallback()
{
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

float SpectralAnalyzer::frequencyToX(float frequency, float width, float minFreq, float maxFreq) const
{
    if (useLinearScale)
    {
        // リニアスケール
        return width * (frequency - minFreq) / (maxFreq - minFreq);
    }
    else
    {
        // 対数スケール
        if (frequency <= 0.0f || minFreq <= 0.0f)
            return 0.0f;
        
        float logMin = std::log10(minFreq);
        float logMax = std::log10(maxFreq);
        float logFreq = std::log10(frequency);
        
        return width * (logFreq - logMin) / (logMax - logMin);
    }
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
        return juce::String(freq / 1000000.0f, 1) + " MHz";
    else if (freq >= 1000.0f)
        return juce::String(freq / 1000.0f, 1) + " kHz";
    else
        return juce::String(static_cast<int>(freq)) + " Hz";
}

void SpectralAnalyzer::drawGrid(juce::Graphics& g, float width, float height, float nyquist)
{
    g.setColour(juce::Colour(60, 60, 70));
    
    float minFreq = useLinearScale ? 0.0f : 10.0f;
    float maxFreq = nyquist;
    
    if (useLinearScale)
    {
        // リニアスケール: 等間隔のグリッド
        float step = nyquist / 10.0f;
        // きりのいい数値に丸める
        if (step > 50000) step = 50000;
        else if (step > 20000) step = 20000;
        else if (step > 10000) step = 10000;
        else if (step > 5000) step = 5000;
        else step = 1000;
        
        for (float freq = step; freq < nyquist; freq += step)
        {
            float x = frequencyToX(freq, width, minFreq, maxFreq);
            g.drawVerticalLine(static_cast<int>(x), 0.0f, height);
        }
    }
    else
    {
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
    
    float minFreq = useLinearScale ? 0.0f : 10.0f;
    float maxFreq = nyquist;
    
    if (useLinearScale)
    {
        // リニアスケール用のラベル
        float step = nyquist / 10.0f;
        if (step > 50000) step = 50000;
        else if (step > 20000) step = 20000;
        else if (step > 10000) step = 10000;
        else if (step > 5000) step = 5000;
        else step = 1000;
        
        for (float freq = step; freq < nyquist; freq += step)
        {
            float x = frequencyToX(freq, width, minFreq, maxFreq);
            juce::String label = formatFrequency(freq);
            
            g.drawText(label, 
                       static_cast<int>(x) - 30, static_cast<int>(height) - 18, 
                       60, 16, juce::Justification::centred);
        }
    }
    else
    {
        // 対数スケール用のラベル
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
    
    float minFreq = useLinearScale ? 0.0f : 10.0f;
    float maxFreq = nyquist;
    float binWidth = sampleRate / static_cast<float>(HyperSonicAnalyzerProcessor::fftSize);
    
    bool pathStarted = false;
    
    // リニアスケールの場合はDC(0Hz)から描画開始
    int startBin = useLinearScale ? 0 : 1;
    
    for (int i = startBin; i < numBins; ++i)
    {
        float frequency = static_cast<float>(i) * binWidth;
        
        if (!useLinearScale && frequency < minFreq)
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
    
    // ラベルを描画
    drawFrequencyLabels(g, width, height, nyquist);
    drawDbLabels(g, width, height);
    
    // サンプルレートとスケール表示
    g.setColour(juce::Colours::yellow);
    g.setFont(12.0f);
    juce::String srText = "Sample Rate: " + formatFrequency(sampleRate);
    g.drawText(srText, static_cast<int>(width) - 180, 5, 175, 20, juce::Justification::right);
    
    // スケール表示
    juce::String scaleText = useLinearScale ? "Linear Scale" : "Log Scale";
    g.setColour(juce::Colours::orange);
    g.drawText(scaleText, static_cast<int>(width) - 180, 22, 175, 20, juce::Justification::right);
}

void SpectralAnalyzer::resized()
{
}
