#include "Game.hpp"

Game::Game()
    : window(sf::VideoMode(800, 600), "GEMS Game"),
    board(8, 8, 60.0f) {

    window.setFramerateLimit(60);

    // Центрируем окно
    sf::Vector2u windowSize = window.getSize();
    float boardWidth = 8 * 60.0f;
    float boardHeight = 8 * 60.0f;
    float offsetX = (windowSize.x - boardWidth) / 2;
    float offsetY = (windowSize.y - boardHeight) / 2;

    // Устанавливаем позицию фона
    // background.setPosition(offsetX, offsetY); // Если нужно
}

void Game::run() {
    while (window.isOpen()) {
        processEvents();
        update();
        render();
    }
}

void Game::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }

        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
                board.handleClick(mousePos);
            }
        }
    }
}

void Game::update() {
    float dt = clock.restart().asSeconds();
    board.update(dt);
}

void Game::render() {
    window.clear(sf::Color(20, 20, 40));
    board.draw(window);
    window.display();
}