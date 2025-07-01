
#include "audiov.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>



//AUDIO ANALYZER///
    void AudioAnalyzer::analyze(const std::int16_t* samples, std::size_t count, AudioData& outData) {
        outData.samples.resize(count);
        for (size_t i = 0; i < count; ++i) {
            outData.samples[i] = samples[i] / 32768.0f; 
        }

        // 2. Вычисление RMS-громкости
        float sumSquares = 0.0f;
        for (float sample : outData.samples) {
            sumSquares += sample * sample;
        }
        outData.rmsVolume = std::sqrt(sumSquares / count);

        // 3. Пиковая громкость
        outData.peakVolume = *std::max_element(
            outData.samples.begin(), 
            outData.samples.end(), 
            [](float a, float b) { return std::abs(a) < std::abs(b); }
        );
    }




//AUDIO//

    void Audio::setVisualizer(Visualizer* viz) {
        visualizer = viz;
    }


    void Audio::load(const sf::SoundBuffer& buffer) {
        samples_arr.assign(buffer.getSamples(), buffer.getSamples() + buffer.getSampleCount());
        current_sample = 0;

        initialize(buffer.getChannelCount(),
                 buffer.getSampleRate(),
                 {sf::SoundChannel::FrontLeft, sf::SoundChannel::FrontRight});
    }


    bool Audio::onGetData(Chunk& data)  {
        data.samples = &samples_arr[current_sample];
        
        if (current_sample + CHUNK_SIZE <= samples_arr.size()) {
            data.sampleCount = CHUNK_SIZE;
            if (visualizer) {
                visualizer->addSamples(data.samples, CHUNK_SIZE);
            }
            current_sample += CHUNK_SIZE;
            return true;
        }
        else {
            data.sampleCount = samples_arr.size() - current_sample;
            if (visualizer && data.sampleCount > 0) {
                visualizer->addSamples(data.samples, data.sampleCount);
            }
            current_sample = 0;
            return false;
        }
    }

    void Audio::onSeek(sf::Time timeOffset)  {
        current_sample = static_cast<std::size_t>(timeOffset.asSeconds() * getSampleRate() * getChannelCount());
    }

    void Audio::pause() {
        sf::SoundStream::pause();
        isPaused = true;
        if (visualizer) visualizer->setPaused(true);
    }

    void Audio::play() {
        sf::SoundStream::play();
        isPaused = false;
        if (visualizer) visualizer->setPaused(false);
    }


//VISUALIZER//
Visualizer::Visualizer() {
    rect_bar.setFillColor(sf::Color::Green);
    rect_bar.setSize(sf::Vector2f(20, 0));
}

void Visualizer::addSamples(const std::int16_t* samples, std::size_t count) {
    std::lock_guard<std::mutex> lock(mtx);
    analyzer.analyze(samples, count, data);
}

void Visualizer::drawBarChart(sf::RenderWindow& window, float &xPos, float &width_column, float distance) {
    std::lock_guard<std::mutex> lock(mtx);
    //проверка на заполненность по горизонтали
    
    if (data.samples.empty()) return;
    // If is paused skip logica for adding the rectangle
    if (!isPaused) {
    if (columns_width >= window_w*0.4) {
    columns.erase(columns.begin());
    }
    columns_width += width_column + distance;
    if (xPos >= window_w) xPos = 0;

    float height =  data.rmsVolume * window_h * 2.0;
    float height_res = height;
    

    if (height >= window_h) height_res = window_h*0.8; // потолок
    
    
    rect_bar.setSize(sf::Vector2f(width_column, height_res));
    rect_bar.setPosition({xPos, window_h-height});
    rect_bar.setFillColor(sf::Color(200, (int(height)%255), (int(height*3)%255)));
    columns.push_back(rect_bar);
    xPos += (width_column + distance);

    }
    for (auto column: columns) {
        window.draw(column);
    }

}

void Visualizer::setPaused(bool isPaused) { this -> isPaused = isPaused;}