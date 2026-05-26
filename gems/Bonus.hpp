#pragma once
#include <SFML/Graphics.hpp>
#include "Gem.hpp"

enum class BonusType {
    Recolor,
    Bomb
};

class Bonus {
public:
    Bonus(BonusType type, int sourceRow, int sourceCol, int targetRow, int targetCol, GemColor sourceColor, float gemSize);

    void update(float dt);
    void draw(sf::RenderWindow& window);
    bool isFinished() const;
    bool isReadyToApply() const;  // Новый метод
    int getTargetRow() const;
    int getTargetCol() const;
    BonusType getType() const;
    GemColor getSourceColor() const;

private:
    BonusType type;
    int sourceRow, sourceCol;
    int targetRow, targetCol;
    GemColor sourceColor;
    float x, y;
    float targetX, targetY;
    float speed;
    float size;
    bool landed;
    bool effectApplied;  // Новое поле
    float timer;
    float alpha;

    sf::CircleShape shape;
};