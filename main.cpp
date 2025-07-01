#include "audiov.h"
#include <optional>
#include <iostream>
#include <cmath>
#include <algorithm>


int main()
{

    // Время
    int secondsCounter = 0;
    sf::Time currentPlayingOffset; // смещение музыки
    //bar chart
    float visualizerX = 0.f;
    float distance_column = -3.f;
    float column_width = 5.f;
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
                    visualizerX = 0;
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