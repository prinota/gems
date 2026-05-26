#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <random>

enum class GemColor {
    Red,
    Blue,
    Green,
    Yellow,
    Purple,
    Orange,
    Count
};

enum class GemState {
    Idle,
    Selected,
    Matched,    // Найден в совпадении
    Falling,    // Падает вниз
    Empty       // Пустая ячейка (после удаления)
};

class Gem {
public:
    Gem(GemColor color, int row, int col, float size);

    void setPosition(float x, float y);
    void draw(sf::RenderWindow& window);
    void setSelected(bool selected);
    bool isSelected() const;
    GemColor getColor() const;
    void setColor(GemColor color);
    int getRow() const;
    int getCol() const;
    void setRow(int row);
    void setCol(int col);
    sf::Vector2f getPosition() const;
    bool contains(sf::Vector2f point) const;
    void animateTo(float targetX, float targetY, float dt);
    bool isAnimating() const;
    void setTargetPosition(float x, float y);
    void setState(GemState state);
    GemState getState() const;
    void setScale(float scale);
    float getScale() const;
    void setAlpha(float alpha);
    float getAlpha() const;
    bool isEmpty() const;

private:
    sf::CircleShape shape;
    GemColor color;
    int row, col;
    bool selected;
    float size;
    sf::Vector2f targetPos;
    bool animating;
    GemState state;
    float currentScale;
    float currentAlpha;

    sf::Color getSFMLColor(GemColor color) const;
};