
#include "audiov.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <functional>


    Button::Button(const sf::Vector2f& size, const sf::Vector2f& position, const sf::Color& fillColor, 
           const sf::Font& font, const std::string& label, const sf::Color& textColor) 
        : onClick(nullptr), text(font), shape(size) {
        shape.setPosition(position);
        shape.setFillColor(fillColor);

        text.setFillColor(textColor);
        text.setString(label);
        centerText();
    }

    Button::~Button() {
    }



    void Button::centerText() {
        sf::FloatRect bounds = shape.getGlobalBounds();
        sf::FloatRect textBounds = text.getGlobalBounds();
        text.setOrigin(textBounds.getCenter());
        text.setPosition(bounds.getCenter());
    }

    
    void Button::setOnClick(const std::function<void()>& callback) {
        std::cout << "click\n";
        onClick = callback;
    }

   
    void Button::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        if (auto Event = event.getIf<sf::Event::MouseButtonPressed>()) {
            sf::Vector2f mousePos = window.mapPixelToCoords(sf::Vector2i(Event->position.x, Event->position.y));
            if (shape.getGlobalBounds().contains(mousePos)) {
                if (isDark(this->shape.getFillColor())) {
                
                this->shape.setFillColor(makeBrighter(this->shape.getFillColor(), 1.5));
                } else 
                {
                    this->shape.setFillColor(makeDarker(this->shape.getFillColor(), 0.5));
                }               
                if (onClick)  {
                    onClick();
                    
                }
            }
        }
    }

 
    void Button::draw(sf::RenderWindow& window) {
        window.draw(shape);
        window.draw(text);
    }


    void Button::setPosition(const sf::Vector2f& pos) {
        shape.setPosition(pos);
        centerText();
    }

    void Button::setFillColor(const sf::Color &color) {
        shape.setFillColor(color);
    }

    sf::Vector2f Button::getPosition() const {
        return shape.getPosition();
    }


//COLOR///
    bool isDark (sf::Color const &color) {
        float brightnes = (0.299f * color.r + 0.587f * color.g + 0.114f * color.b) / 255.0f;
        return (brightnes < 0.5);

    }

    sf::Color makeBrighter(const sf::Color& color, float factor) {
        std::cout << "br\n";
        return sf::Color(
            std::min(255, static_cast<int>(color.r * factor)),
            std::min(255, static_cast<int>(color.g * factor)),
            std::min(255, static_cast<int>(color.b * factor)),
            color.a
        );
    }

    sf::Color makeDarker(const sf::Color& color, float factor ) {
        std::cout << "dr\n";
        return sf::Color(
            std::max(0, static_cast<int>(color.r * factor)),
            std::max(0, static_cast<int>(color.g * factor)),
            std::max(0, static_cast<int>(color.b * factor)),
            color.a
        );
    }


//AUDIO ANALYZER///
    void AudioAnalyzer::analyze(const std::int16_t* samples, std::size_t count, AudioData& outData) {
        // 0-1 samples
        outData.samples.resize(count);
        for (size_t i = 0; i < count; ++i) {
            outData.samples[i] = samples[i] / 32768.0f; 
        }

        //  RMS-громкость
        float sumSquares = 0.0f;
        for (float sample : outData.samples) {
            sumSquares += sample * sample;
        }
        outData.rmsVolume = std::sqrt(sumSquares / count);

        //  Пиковая громкость
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
    if (!BarChartIsActive) return;
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

void Visualizer::drawCircle(sf::RenderWindow &window, float radius) {
    if (!CircleIsActive) return;
    sf::CircleShape circle;
    float outline_size = data.rmsVolume * window_h / 5;
    smoothed_outline = smooth_alpha * outline_size + (1 - smooth_alpha) * smoothed_outline;
    radius += smoothed_outline * 0.4;
    if (smoothed_outline >= radius*0.4) smoothed_outline = radius*0.4;
    if (smoothed_outline >= radius*0.25) smoothed_outline*=0.6;
    circle.setOutlineThickness(outline_size);
    circle.setOrigin({radius/2,radius/2});
    circle.setPosition({window_w/2, window_h/2});
    circle.setOutlineColor(sf::Color(100, (int(smoothed_outline*10)%255), (int(smoothed_outline*3)%255)));
    circle.setFillColor(sf::Color((int(smoothed_outline*10)%255), (int(smoothed_outline*3)%255), 100));
    circle.setRadius(radius);
    window.draw(circle);
}

//x(t) = Ax * sin(w.xt+fi.x)
//y(t) = Ay * sin(w.yt+fi.y)
void Visualizer::drawLissajous(sf::RenderWindow &window) {
    if (!LissajousIsActive) return;
    std::vector<sf::Vector2f> points;
    points.reserve(100);
    int quantity_points = 100;
    
    // переменные уравнения
    float freqX = int(10 * data.peakVolume);  
    float freqY = int(10 * data.rmsVolume);    
    float amplitude = 100.0f + data.rmsVolume * 400.0f;
    
    sf::Color curveColor(
        static_cast<std::int8_t>(150 + data.peakVolume * 105),
        static_cast<std::int8_t>(50 + data.rmsVolume * 205),
        200
    );
    
    // Генерация точек кривой
    for (int i = 0; i <= quantity_points; ++i) {
        float t = (float)i / quantity_points * 2.0f * PI;
        float x = window_w/2 + amplitude * std::sin(freqX * t + PI / int(data.peakVolume*2/data.rmsVolume));
        float y = window_h/2 + amplitude * std::sin(freqY * t);
        points.emplace_back(sf::Vector2f{x,y});
    }
    
    for (size_t i = 1; i < points.size(); ++i) drawThickLine(window, points[i-1], points[i], 5.0f, curveColor);
    

}


void Visualizer::drawThickLine(sf::RenderWindow& window, sf::Vector2f p1, sf::Vector2f p2, float thickness, sf::Color color) {
    sf::Vector2f dir = p2 - p1;
    float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    sf::RectangleShape line(sf::Vector2f(length, thickness));
    line.setPosition(p1);
    line.setRotation(sf::degrees(std::atan2(dir.y, dir.x) * 180 / 3.14159f));
    line.setFillColor(color);
    window.draw(line);
}

void Visualizer::setPaused(bool isPaused) { this -> isPaused = isPaused;}

/// КОНЕЦ КЛАССОВ. ПРОСТО ФУНКЦИИ 
// Список кнопок
void ShowButtonsList (sf::RenderWindow &window,std::vector<Button> &list) {
    for (auto button: list) {
        button.draw(window);
    }

}


void mainWindowCycle(sf::RenderWindow& window, Audio& music, Visualizer& viz, 
                   std::vector<Button>& buttons, sf::Text& MusicText,
                   float& visualizerX, float column_width, 
                   float distance_column, float circle_radius,
                   sf::Time& currentPlayingOffset)
{
    int secondsCounter = 0;
    while (auto event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>()) {
            window.close();
            return;
        }

        for (auto& button : buttons) {
            button.handleEvent(*event, window);
        }

        if (auto keyEvent = event->getIf<sf::Event::KeyPressed>())
        {
            sf::SoundSource::Status status = music.getStatus();
            if (keyEvent->code == sf::Keyboard::Key::Space)
            {
                if (status == sf::SoundStream::Status::Paused) {
                    music.setPlayingOffset(currentPlayingOffset);
                    music.play();
                }
                else if (status == sf::SoundStream::Status::Playing) {
                    currentPlayingOffset = music.getPlayingOffset();
                    music.pause();
                }
            }
            else if (keyEvent->code == sf::Keyboard::Key::R) {
                music.setPlayingOffset(sf::seconds(0.f));
                visualizerX = 0;
                currentPlayingOffset = sf::seconds(0.f);
                music.play();
            }
        }
    }

    // Обновление статуса музыки
    sf::String StatusText;
    switch(static_cast<int>(music.getStatus())) {
        case 0: StatusText = "Music is stopped"; break;
        case 1: StatusText = "Music is paused"; break;
        case 2: StatusText = "Music is playing"; break;
    }

    if (music.getPlayingOffset().asSeconds() >= secondsCounter && 
        !(music.getStatus() == sf::Sound::Status::Paused))
    {
        currentPlayingOffset = sf::seconds(music.getPlayingOffset().asSeconds());
    }

    // Отрисовка
    MusicText.setString(StatusText + "\n Time " + std::to_string(currentPlayingOffset.asSeconds()) + "s\n");
    window.clear(sf::Color::Black);
    viz.drawBarChart(window, visualizerX, column_width, distance_column);
    viz.drawCircle(window, circle_radius);
    viz.drawLissajous(window);
    window.draw(MusicText);
    ShowButtonsList(window, buttons);
    window.display();
}

void menuWindowCycle(sf::RenderWindow& window, std::vector<Button>& buttons)
{
    while (auto event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>()) {
            window.close();
            return;
        }

        
        for (auto &button: buttons) {
            button.handleEvent(*event, window);
        }
    }

    window.clear(sf::Color::Black);
    ShowButtonsList(window, buttons);
    window.display();

}