// visualizer.h
#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <mutex>

inline constexpr unsigned int window_w = 1600;
inline constexpr unsigned int window_h = 800;
inline constexpr float PI = 3.1415926535;

struct AudioData {
    std::vector<float> samples;      // Нормализованные семплы [-1, 1]
    float rmsVolume = 0.0f;          // Среднеквадратичная громкость
    float peakVolume = 0.0f;         // Пиковая громкость
};

//color
bool isDark (sf::Color const &color);
sf::Color makeBrighter(const sf::Color& color, float factor = 1.2f);
sf::Color makeDarker(const sf::Color& color, float factor = 0.8f);

class Button {
private:
    sf::RectangleShape shape;      
    sf::Text text;         
    std::function<void()> onClick; 

public:
    //Button(sf::Shape* shape, const sf::Font& font, const std::string& label, const sf::Color& textColor);
    Button(const sf::Vector2f& size, const sf::Vector2f& position, const sf::Color& fillColor, 
           const sf::Font& font, const std::string& label, const sf::Color& textColor);
    Button(const Button&) = default;
    Button& operator=(const Button&) = default;
    ~Button();

    void centerText();
    void setOnClick(const std::function<void()>& callback);
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void draw(sf::RenderWindow& window);
    void setPosition(const sf::Vector2f& pos);
    void setFillColor(const sf::Color& color);
    sf::Vector2f getPosition() const;
};

class AudioAnalyzer {
public:
    void analyze(const std::int16_t* samples, std::size_t count, AudioData& outData);
};

class Visualizer {
    private:
    bool isPaused; // состоянии музыки
    AudioData data; // вся инфа о музыке
    AudioAnalyzer analyzer; // обьект анализатор
    mutable std::mutex mtx;
    // диаграмма
    float columns_width = 0;
    std::vector<sf::RectangleShape> columns;
    sf::RectangleShape rect_bar;
    // круг (экспоненциальное сглаживание)
    float smoothed_outline = 100.f;
    float smooth_alpha = 0.1;

public:
    bool BarChartIsActive = false;
    bool CircleIsActive = false;
    bool LissajousIsActive = false;
    Visualizer();
    void addSamples(const std::int16_t* samples, std::size_t count);
    void drawBarChart(sf::RenderWindow& window, float &xPos, float &width_column, float distance);
    void drawCircle(sf::RenderWindow &window, float radius); 
    void drawLissajous(sf::RenderWindow &window);
    void drawThickLine(sf::RenderWindow& window, sf::Vector2f p1, sf::Vector2f p2, float thickness, sf::Color color);
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

void ShowButtonsList (sf::RenderWindow &window, std::vector<Button> &list);

void mainWindowCycle(sf::RenderWindow& window, Audio& music, Visualizer& viz, 
                    std::vector<Button>& buttons, sf::Text& MusicText,
                    float& visualizerX, float column_width, 
                    float distance_column, float circle_radius,
                    sf::Time& currentPlayingOffset);

void menuWindowCycle(sf::RenderWindow& window, std::vector<Button>& buttons);

