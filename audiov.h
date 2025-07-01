// visualizer.h
#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <mutex>

// Объявляем переменные как extern (определение будет в .cpp)
inline constexpr unsigned int window_w = 1600;
inline constexpr unsigned int window_h = 800;

struct AudioData {
    std::vector<float> samples;      // Нормализованные семплы [-1, 1]
    float rmsVolume = 0.0f;          // Среднеквадратичная громкость
    float peakVolume = 0.0f;         // Пиковая громкость
};

class AudioAnalyzer {
public:
    void analyze(const std::int16_t* samples, std::size_t count, AudioData& outData);
};

class Visualizer {
private:
    bool isPaused;
    AudioData data;
    AudioAnalyzer analyzer;
    float columns_width = 0;
    std::vector<sf::RectangleShape> columns;
    sf::RectangleShape rect_bar;
    mutable std::mutex mtx;

public:
    Visualizer();
    void addSamples(const std::int16_t* samples, std::size_t count);
    void drawBarChart(sf::RenderWindow& window, float &xPos, float &width_column, float distance);
    void setPaused(bool isPaused);
};

class Audio : public sf::SoundStream {
private:
    bool isPaused;
    std::vector<std::int16_t> samples_arr;
    std::size_t current_sample = 0;
    Visualizer* visualizer = nullptr;
    static constexpr int CHUNK_SIZE = 1000;

public:
    void setVisualizer(Visualizer* viz);
    void load(const sf::SoundBuffer& buffer);
    void pause();
    void play();

private:
    bool onGetData(Chunk& data) override;
    void onSeek(sf::Time timeOffset) override;
};