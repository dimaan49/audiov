#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <mutex>
#include <optional>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

unsigned int window_w = 1600;
unsigned int window_h = 800;



struct AudioData {
    std::vector<float> samples;      // Нормализованные семплы [-1, 1]
    float rmsVolume = 0.0f;          // Среднеквадратичная громкость
    float peakVolume = 0.0f;         // Пиковая громкость
    
};


class AudioAnalyzer {
public:
    void analyze(const std::int16_t* samples, std::size_t count, AudioData& outData) {
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
};


class Visualizer {
private:
    float columns_width = 0;
    std::vector<sf::RectangleShape> columns;
    std::vector<float> samples_arr;
    sf::RectangleShape rect_bar;
    mutable std::mutex mtx;

public:
    Visualizer();
    void addSamples(const std::int16_t* samples, std::size_t count);
    void drawBarChart(sf::RenderWindow& window, float &xPos, float &width_column, float distance);
};

class Audio : public sf::SoundStream {
private:
    std::vector<std::int16_t> samples_arr;
    std::size_t current_sample = 0;
    Visualizer* visualizer = nullptr;
    static constexpr int CHUNK_SIZE = 1000;

public:
    void setVisualizer(Visualizer* viz) {
        visualizer = viz;
    }

    void load(const sf::SoundBuffer& buffer) {
        samples_arr.assign(buffer.getSamples(), buffer.getSamples() + buffer.getSampleCount());
        current_sample = 0;

        initialize(buffer.getChannelCount(),
                 buffer.getSampleRate(),
                 {sf::SoundChannel::FrontLeft, sf::SoundChannel::FrontRight});
    }

private:
    bool onGetData(Chunk& data) override {
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

    void onSeek(sf::Time timeOffset) override {
        current_sample = static_cast<std::size_t>(timeOffset.asSeconds() * getSampleRate() * getChannelCount());
    }
};

Visualizer::Visualizer() {
    rect_bar.setFillColor(sf::Color::Green);
    rect_bar.setSize(sf::Vector2f(20, 0));
}

void Visualizer::addSamples(const std::int16_t* samples, std::size_t count) {
    std::lock_guard<std::mutex> lock(mtx);
    samples_arr.clear();
    samples_arr.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        samples_arr.push_back(samples[i] / 32768.f); 
    }
}

void Visualizer::drawBarChart(sf::RenderWindow& window, float &xPos, float &width_column, float distance) {
    std::lock_guard<std::mutex> lock(mtx);
    //проверка на заполненность по горизонтали
    if (samples_arr.empty()) return;
    if (columns_width >= window_w*0.4) {
    columns.erase(columns.begin());
    }
    columns_width += width_column + distance;
    if (xPos >= window_w) xPos = 0;

    float sum = 0;
    for (float sample : samples_arr) {  
        sum += sample;
    }
    float height = std::abs(sum) * 10;
    float height_res = height;

    if (height >= window_h) {
        height_res = window_h*0.8;
    } else if (height <= window_h*0.1) {
        height_res += 10;
        height_res *= 10;
    }
    
    rect_bar.setSize(sf::Vector2f(width_column, height_res));
    rect_bar.setPosition({xPos, window_h-height});
    rect_bar.setFillColor(sf::Color(150, (int(height)%255), (int(height*2)%255)));
    columns.push_back(rect_bar);
    xPos += (width_column + distance);
    for (auto column: columns) {
        window.draw(column);
    }

};





int main()
{

    // Время
    int secondsCounter = 0;
    sf::Time currentPlayingOffset;
    float visualizerX = 0.f;
    float distance_column = 0.f;
    float column_width = 1.f;
    Visualizer viz;


    sf::RenderWindow window(sf::VideoMode({window_w, window_h}), "AudioGraphic");


    // Загружаем шрифт alger
    sf::Font StatusFont;
    if (!StatusFont.openFromFile("ALGER.TTF"))
    {
        return 1;
    }

    //Текст
    sf::String StatusText = "";
    sf::Text MusicText(StatusFont);
    MusicText.setCharacterSize(60);
    MusicText.setOrigin(MusicText.getGlobalBounds().getCenter());
    MusicText.setPosition({float(window_w/3), float(window_h/10)});
    MusicText.setFillColor(sf::Color::Magenta);


    // Загружаем звуковой буфер (новый стиль SFML 3.0)
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile("noch.ogg")) {
        return 1;
    }
    Audio music;
    music.load(buffer);
    music.setVisualizer(&viz);
    music.play();


    while (window.isOpen())
    {


        while (std::optional event = window.pollEvent())
        {

            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }


            if (auto keyEvent = event->getIf<sf::Event::KeyPressed>())
            {
                sf::SoundSource::Status status = music.getStatus();
                if ((keyEvent->code == sf::Keyboard::Key::Space) && status == sf::SoundStream::Status::Paused)
                {
                    music.setPlayingOffset(currentPlayingOffset);
                    music.play(); 
                }
                if ((keyEvent->code == sf::Keyboard::Key::Space) && status == sf::SoundStream::Status::Playing)
                {
                    currentPlayingOffset = music.getPlayingOffset();
                    music.pause(); 
                }
                if (keyEvent->code == sf::Keyboard::Key::R) 
                {
                    music.setPlayingOffset(sf::seconds(0.f));
                    currentPlayingOffset = sf::seconds(0.f);
                    music.play();
                }
            }
        }
        sf::SoundStream::Status status = music.getStatus();
            switch(static_cast<int>(status)) {
                case 0: 
                    StatusText = "Music is stopped";
                    break;
                case 1:
                    StatusText = "Music is paused";
                    break;
                case 2:
                    StatusText = "Music is playing";
                    break;  
            }
        if (music.getPlayingOffset().asSeconds() >= secondsCounter && !(music.getStatus() == sf::Sound::Status::Paused))
        {
            currentPlayingOffset = sf::Time(sf::seconds(music.getPlayingOffset().asSeconds()));
        }

        MusicText.setString(StatusText +"\n Time " + std::to_string(currentPlayingOffset.asSeconds())+"s\n");
        window.clear(sf::Color::Black);
        viz.drawBarChart(window, visualizerX, column_width, distance_column );
        window.draw(MusicText);
        window.display();
    }

    return 0;
}