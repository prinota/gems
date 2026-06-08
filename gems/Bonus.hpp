#pragma once
#include <SFML/Graphics.hpp>
#include "Gem.hpp"
#include <vector>
#include <memory>

enum class BonusType {
    Recolor,
    Bomb
};

class Bonus {
public:
    Bonus(BonusType type, int sourceRow, int sourceCol, int targetRow, int targetCol,
        GemColor sourceColor, float gemSize);
    virtual ~Bonus() = default;

    void update(float dt);
    void draw(sf::RenderWindow& window) const;

    virtual void applyEffect(std::vector<std::vector<std::unique_ptr<Gem>>>& gems,
        int rows, int cols) = 0;

    bool isReadyToApply() const { return readyToApply && !effectApplied; }
    bool isFinished() const { return effectApplied; }
    void markEffectApplied() { effectApplied = true; }

    int getTargetRow() const { return targetRow; }
    int getTargetCol() const { return targetCol; }
    BonusType getType() const { return type; }
    GemColor getSourceColor() const { return sourceColor; }

protected:
    BonusType type;
    int sourceRow, sourceCol;
    int targetRow, targetCol;
    GemColor sourceColor;

    float x, y;
    float targetX, targetY;
    float speed;
    float size;
    bool landed;
    bool readyToApply;
    bool effectApplied;
    float timer;
    float alpha;

    sf::CircleShape shape;

    virtual void onLanded() { readyToApply = true; }
};

class RecolorBonus : public Bonus {
public:
    RecolorBonus(int sourceRow, int sourceCol, int targetRow, int targetCol,
        GemColor sourceColor, float gemSize);

    void applyEffect(std::vector<std::vector<std::unique_ptr<Gem>>>& gems,
        int rows, int cols) override;
};

class BombBonus : public Bonus {
public:
    BombBonus(int sourceRow, int sourceCol, int targetRow, int targetCol,
        GemColor sourceColor, float gemSize);

    void applyEffect(std::vector<std::vector<std::unique_ptr<Gem>>>& gems,
        int rows, int cols) override;
};