#include "audiov.h"
#include <optional>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <thread>


int main()
{

    // Время
    int secondsCounter = 0;
    sf::Time currentPlayingOffset; // смещение музыки
    //bar chart
    float visualizerX = 0.f;
    float distance_column = 0.f;
    float column_width = 1.f;
    Visualizer viz;
    // круг
    float circle_radius = 100.f;
    // меню
    std::vector<Button> main_buttons; // 1. вектор кнопок
    std::vector<Button> menu_buttons;



    sf::RenderWindow menu_window(sf::VideoMode({window_w, window_h}), "Menu Window");
    sf::RenderWindow main_window(sf::VideoMode({window_w, window_h}), "AudioGraphic"); //main window
    menu_window.setVisible(false);


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


    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile("noch.ogg")) {
        return 1;
    }
    Audio music;
    music.load(buffer);
    music.setVisualizer(&viz);
    music.play();

        // кнопка
        
    std::string sbut1 = "Bar Chart\n";
    std::string sbut2 = "Circle\n";
    std::string sbut3 = "Lissajous\n";

    Button but1({200.f,100.f}, {0, window_h/2-110.f}, sf::Color::Yellow, StatusFont, sbut1, sf::Color::Magenta);
    Button but2({200.f,100.f}, {0, window_h/2}, sf::Color::Yellow, StatusFont, sbut2, sf::Color::Magenta);
    Button but3({200.f,100.f}, {0, window_h/2+110.f}, sf::Color::Yellow, StatusFont, sbut3, sf::Color::Magenta);
    
    std::function<void()> f1 = [&viz]() { viz.BarChartIsActive = !viz.BarChartIsActive; };
    std::function<void()> f2 = [&viz]() { viz.CircleIsActive = !viz.CircleIsActive; };
    std::function<void()> f3 = [&viz]() { viz.LissajousIsActive = !viz.LissajousIsActive; };

    but1.setOnClick(f1);
    but2.setOnClick(f2);
    but3.setOnClick(f3);

    menu_buttons.push_back(but1);
    menu_buttons.push_back(but2);
    menu_buttons.push_back(but3);

    // button to switch windows
    bool main_visible = true;
    Button win_switcher({100.f, 50.f}, {10.f, 10.f}, sf::Color::Green, StatusFont, "switch", sf::Color::White);
    std::function<void()> f_switch = [&main_window, &menu_window, &main_visible, &music]() { 
        main_window.setVisible(!main_visible);
        menu_window.setVisible(main_visible);
        main_visible = !main_visible;
        if (main_visible) {
            main_window.requestFocus();
            music.play();
            
        } 
        else 
        {
            menu_window.requestFocus();
            music.pause();
        }
        
        };
    
    win_switcher.setOnClick(f_switch); 
    main_buttons.push_back(win_switcher);
    menu_buttons.push_back(win_switcher);




       while (main_window.isOpen() || menu_window.isOpen())
    {
        if (main_visible) {
            mainWindowCycle(main_window, music, viz, main_buttons, MusicText,
                           visualizerX, column_width, distance_column, circle_radius, currentPlayingOffset);
        } else {
            menuWindowCycle(menu_window, menu_buttons);
        }
    }

    return 0;
}


