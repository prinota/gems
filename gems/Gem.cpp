#include "Gem.hpp"
#include "GameBoard.hpp"
#include <cmath>
#include <cstdlib>

Gem::Gem(GemColor color, int row, int col, float size)
    : color(color), row(row), col(col), selected(false), size(size),
    animating(false), state(GemState::Idle), currentScale(1.0f), currentAlpha(255.0f),
    hasSymbol(false) {
    shape.setRadius(size / 2 - 2);
    shape.setPointCount(4);
    shape.setFillColor(getSFMLColor(color));
    shape.setOutlineThickness(2);
    shape.setOutlineColor(sf::Color::White);
    shape.setOrigin(size / 2, size / 2);
}

sf::Color Gem::getSFMLColor(GemColor color) const {
    switch (color) {
    case GemColor::Red:    return sf::Color(255, 60, 60);
    case GemColor::Blue:   return sf::Color(60, 60, 255);
    case GemColor::Green:  return sf::Color(60, 255, 60);
    case GemColor::Yellow: return sf::Color(255, 255, 60);
    case GemColor::Purple: return sf::Color(200, 60, 255);
    case GemColor::Orange: return sf::Color(255, 160, 60);
    default:               return sf::Color::White;
    }
}

void Gem::initSymbol(const sf::Color& symbolColor) {
    hasSymbol = true;
    symbolShape.setRadius(size * 0.15f);
    symbolShape.setOrigin(size * 0.15f, size * 0.15f);
    symbolShape.setFillColor(symbolColor);
    symbolShape.setOutlineColor(sf::Color::Black);
    symbolShape.setOutlineThickness(1);
}

void Gem::drawSymbol(sf::RenderWindow& window) const {
    if (hasSymbol && state != GemState::Empty) {
        sf::Vector2f pos = shape.getPosition();
        symbolShape.setPosition(pos.x, pos.y);
        window.draw(symbolShape);
    }
}

void Gem::setPosition(float x, float y) {
    shape.setPosition(x, y);
    targetPos = sf::Vector2f(x, y);
}

void Gem::draw(sf::RenderWindow& window) {
    if (state == GemState::Empty) return;

    shape.setScale(currentScale, currentScale);
    sf::Color fillColor = shape.getFillColor();
    fillColor.a = static_cast<sf::Uint8>(currentAlpha);
    shape.setFillColor(fillColor);

    sf::Color outlineColor = shape.getOutlineColor();
    outlineColor.a = static_cast<sf::Uint8>(currentAlpha);
    shape.setOutlineColor(outlineColor);

    window.draw(shape);
    drawSymbol(window);

    fillColor.a = 255;
    shape.setFillColor(fillColor);
    outlineColor.a = 255;
    shape.setOutlineColor(outlineColor);
}

void Gem::setSelected(bool selected) {
    this->selected = selected;
    shape.setOutlineColor(selected ? sf::Color::Yellow : sf::Color::White);
}

bool Gem::isSelected() const { return selected; }
GemColor Gem::getColor() const { return color; }

void Gem::setColor(GemColor color) {
    this->color = color;
    shape.setFillColor(getSFMLColor(color));
}

int Gem::getRow() const { return row; }
int Gem::getCol() const { return col; }
void Gem::setRow(int row) { this->row = row; }
void Gem::setCol(int col) { this->col = col; }

sf::Vector2f Gem::getPosition() const {
    return shape.getPosition();
}

bool Gem::contains(sf::Vector2f point) const {
    return shape.getGlobalBounds().contains(point);
}

void Gem::animateTo(float targetX, float targetY, float dt) {
    sf::Vector2f currentPos = shape.getPosition();
    sf::Vector2f direction(targetX - currentPos.x, targetY - currentPos.y);
    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

    if (distance < 0.5f) {
        shape.setPosition(targetX, targetY);
        animating = false;
    }
    else {
        float speed = 800.0f;
        sf::Vector2f velocity = direction / distance * speed;

        if (speed * dt >= distance) {
            shape.setPosition(targetX, targetY);
            animating = false;
        }
        else {
            shape.move(velocity * dt);
        }
    }
}

bool Gem::isAnimating() const { return animating; }

void Gem::setTargetPosition(float x, float y) {
    targetPos = sf::Vector2f(x, y);
    animating = true;
}

void Gem::setState(GemState state) {
    this->state = state;
}

GemState Gem::getState() const {
    return state;
}

void Gem::setScale(float scale) {
    currentScale = scale;
}

float Gem::getScale() const {
    return currentScale;
}

void Gem::setAlpha(float alpha) {
    currentAlpha = alpha;
}

float Gem::getAlpha() const {
    return currentAlpha;
}

bool Gem::isEmpty() const {
    return state == GemState::Empty;
}

// StandardGem
StandardGem::StandardGem(GemColor color, int row, int col, float size)
    : Gem(color, row, col, size) {
}

// RecolorGem
RecolorGem::RecolorGem(GemColor color, int row, int col, float size)
    : Gem(color, row, col, size), specialActive(true) {
    shape.setOutlineColor(sf::Color(255, 215, 0));
    shape.setOutlineThickness(3);
    initSymbol(sf::Color::White);
}

void RecolorGem::activateEffect(GameBoard& board) {
    if (specialActive) {
        board.recolorRandomGems(row, col, color, 3);
        specialActive = false;
    }
}

void RecolorGem::deactivateSpecial() {
    specialActive = false;
    shape.setOutlineColor(sf::Color::White);
    shape.setOutlineThickness(2);
    hasSymbol = false;
}

// BombGem
BombGem::BombGem(GemColor color, int row, int col, float size)
    : Gem(color, row, col, size), specialActive(true) {
    shape.setOutlineColor(sf::Color(255, 50, 50));
    shape.setOutlineThickness(3);
    initSymbol(sf::Color(255, 200, 0));
}

void BombGem::activateEffect(GameBoard& board) {
    if (specialActive) {
        board.destroyGemsInRadius(row, col, 2);
        specialActive = false;
    }
}

void BombGem::deactivateSpecial() {
    specialActive = false;
    shape.setOutlineColor(sf::Color::White);
    shape.setOutlineThickness(2);
    hasSymbol = false;
}

// GemFactory
std::unique_ptr<Gem> GemFactory::createRandomGem(int row, int col, float size) {
    int colorIndex = std::rand() % static_cast<int>(GemColor::Count);
    GemColor color = static_cast<GemColor>(colorIndex);
    return std::make_unique<StandardGem>(color, row, col, size);
}

std::unique_ptr<Gem> GemFactory::createSpecialGem(GemType type, GemColor color, int row, int col, float size) {
    switch (type) {
    case GemType::Recolor:
        return std::make_unique<RecolorGem>(color, row, col, size);
    case GemType::Bomb:
        return std::make_unique<BombGem>(color, row, col, size);
    default:
        return std::make_unique<StandardGem>(color, row, col, size);
    }
}