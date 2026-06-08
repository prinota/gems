#include "Bonus.hpp"
#include <cmath>
#include <algorithm>

Bonus::Bonus(BonusType type, int sourceRow, int sourceCol, int targetRow, int targetCol,
    GemColor sourceColor, float gemSize)
    : type(type), sourceRow(sourceRow), sourceCol(sourceCol),
    targetRow(targetRow), targetCol(targetCol), sourceColor(sourceColor),
    speed(400.0f), size(gemSize * 0.7f), landed(false), readyToApply(false),
    effectApplied(false), timer(0.0f), alpha(255.0f) {

    x = sourceCol * gemSize + gemSize / 2;
    y = sourceRow * gemSize + gemSize / 2;

    targetX = targetCol * gemSize + gemSize / 2;
    targetY = targetRow * gemSize + gemSize / 2;

    shape.setRadius(size / 2);
    shape.setPointCount(4);
    shape.setOrigin(size / 2, size / 2);
    shape.setPosition(x, y);
    shape.setOutlineThickness(3);

    if (type == BonusType::Recolor) {
        shape.setFillColor(sf::Color(255, 215, 0, 255)); // Золотой
        shape.setOutlineColor(sf::Color(255, 255, 255, 255));
    }
    else {
        shape.setFillColor(sf::Color(255, 50, 50, 255)); // Красный
        shape.setOutlineColor(sf::Color(255, 200, 0, 255));
    }
}

void Bonus::update(float dt) {
    timer += dt;

    if (!landed) {
        float dx = targetX - x;
        float dy = targetY - y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance < 3.0f) {
            x = targetX;
            y = targetY;
            landed = true;
            timer = 0.0f; //Сбрасываем таймер для фазы мигания
        }
        else {
            x += (dx / distance) * speed * dt;
            y += (dy / distance) * speed * dt;
        }

        shape.setPosition(x, y);
    }
    else if (!readyToApply) {
        float blink = std::abs(std::sin(timer * 20.0f));
        alpha = 128.0f + 127.0f * blink;

        sf::Color fillColor = shape.getFillColor();
        fillColor.a = static_cast<sf::Uint8>(alpha);
        shape.setFillColor(fillColor);

        if (timer >= 0.5f) {
            onLanded(); 
        }
    }
}

void Bonus::draw(sf::RenderWindow& window) const {
    if (isFinished()) return;
    window.draw(shape);
}

RecolorBonus::RecolorBonus(int sourceRow, int sourceCol, int targetRow, int targetCol,
    GemColor sourceColor, float gemSize)
    : Bonus(BonusType::Recolor, sourceRow, sourceCol, targetRow, targetCol, sourceColor, gemSize) {
}

void RecolorBonus::applyEffect(std::vector<std::vector<std::unique_ptr<Gem>>>& gems,
    int rows, int cols) {
    if (targetRow >= 0 && targetRow < rows && targetCol >= 0 && targetCol < cols) {
        if (gems[targetRow][targetCol] && !gems[targetRow][targetCol]->isEmpty()) {
            gems[targetRow][targetCol]->setColor(sourceColor);
        }
    }
}

BombBonus::BombBonus(int sourceRow, int sourceCol, int targetRow, int targetCol,
    GemColor sourceColor, float gemSize)
    : Bonus(BonusType::Bomb, sourceRow, sourceCol, targetRow, targetCol, sourceColor, gemSize) {
}

void BombBonus::applyEffect(std::vector<std::vector<std::unique_ptr<Gem>>>& gems,
    int rows, int cols) {
    if (targetRow >= 0 && targetRow < rows && targetCol >= 0 && targetCol < cols) {
        if (gems[targetRow][targetCol] && !gems[targetRow][targetCol]->isEmpty()) {
            gems[targetRow][targetCol]->setState(GemState::Empty);
        }
    }
}