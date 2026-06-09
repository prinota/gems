#pragma once
#include <SFML/Graphics.hpp>
#include "GameBoard.hpp"

class Game {
public:
    Game();
    void run();

private:
    sf::RenderWindow window;
    GameBoard board;
    sf::Clock clock;

    void processEvents();
    void update();
    void render();
};