#include "Bonus.hpp"
#include <cmath>

Bonus::Bonus(BonusType type, int sourceRow, int sourceCol, int targetRow, int targetCol,
    GemColor sourceColor, float gemSize)
    : type(type), sourceRow(sourceRow), sourceCol(sourceCol),
    targetRow(targetRow), targetCol(targetCol), sourceColor(sourceColor),
    speed(400.0f), size(gemSize * 0.7f), landed(false), effectApplied(false),
    timer(0.0f), alpha(255.0f) {

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
        // Летим к цели
        float dx = targetX - x;
        float dy = targetY - y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance < 3.0f) {
            x = targetX;
            y = targetY;
            landed = true;
        }
        else {
            x += (dx / distance) * speed * dt;
            y += (dy / distance) * speed * dt;
        }

        shape.setPosition(x, y);
    }
    else if (!effectApplied) {
        // Мигаем на месте 0.5 секунды
        float blink = std::abs(std::sin(timer * 20.0f));
        alpha = 128.0f + 127.0f * blink;

        sf::Color fillColor = shape.getFillColor();
        fillColor.a = static_cast<sf::Uint8>(alpha);
        shape.setFillColor(fillColor);

        // Автоматически применяем эффект через 0.5 секунды
        if (timer > 0.5f) {
            effectApplied = true;
        }
    }
}

void Bonus::draw(sf::RenderWindow& window) {
    if (isFinished()) return;
    window.draw(shape);
}

bool Bonus::isFinished() const {
    return effectApplied && timer > 0.6f;
}

bool Bonus::isReadyToApply() const {
    return effectApplied && !isFinished();
}

int Bonus::getTargetRow() const { return targetRow; }
int Bonus::getTargetCol() const { return targetCol; }
BonusType Bonus::getType() const { return type; }
GemColor Bonus::getSourceColor() const { return sourceColor; }