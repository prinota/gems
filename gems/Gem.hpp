#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>

enum class GemColor {
    Red, Blue, Green, Yellow, Purple, Orange, Count
};

enum class GemState {
    Idle,
    Matched,
    Falling,
    Empty,
    BonusMoving
};

enum class GemType {
    Standard,
    Recolor,
    Bomb
};

class GameBoard;

class Gem {
public:
    Gem(GemColor color, int row, int col, float size);
    virtual ~Gem() = default;

    virtual void draw(sf::RenderWindow& window);
    virtual void animateTo(float targetX, float targetY, float dt);
    virtual bool isSpecial() const { return false; }
    virtual void activateEffect(GameBoard& board) {}
    virtual void deactivateSpecial() {}
    virtual GemType getType() const { return GemType::Standard; }  

    void setPosition(float x, float y);
    bool contains(sf::Vector2f point) const;
    sf::Vector2f getPosition() const;

    void setSelected(bool selected);
    bool isSelected() const;

    GemColor getColor() const;
    void setColor(GemColor color);

    int getRow() const;
    int getCol() const;
    void setRow(int row);
    void setCol(int col);

    void setTargetPosition(float x, float y);
    bool isAnimating() const;

    void setState(GemState state);
    GemState getState() const;
    bool isEmpty() const;

    void setScale(float scale);
    float getScale() const;

    void setAlpha(float alpha);
    float getAlpha() const;

protected:
    GemColor color;
    int row, col;
    bool selected;
    float size;
    bool animating;
    GemState state;
    float currentScale;
    float currentAlpha;
    sf::Vector2f targetPos;
    sf::CircleShape shape;

    sf::CircleShape symbolShape;
    bool hasSymbol;

    sf::Color getSFMLColor(GemColor color) const;
    void initSymbol(const sf::Color& symbolColor);
    void drawSymbol(sf::RenderWindow& window);
};


class RecolorGem : public Gem {
public:
    RecolorGem(GemColor color, int row, int col, float size);
    bool isSpecial() const override { return true; }
    void activateEffect(GameBoard& board) override;
    void deactivateSpecial() override;
    GemType getType() const override { return GemType::Recolor; }

private:
    bool specialActive = true;
};

class BombGem : public Gem {
public:
    BombGem(GemColor color, int row, int col, float size);
    bool isSpecial() const override { return true; }
    void activateEffect(GameBoard& board) override;
    void deactivateSpecial() override;
    GemType getType() const override { return GemType::Bomb; }

private:
    bool specialActive = true;
};

class GemFactory {
public:
    static std::unique_ptr<Gem> createRandomGem(int row, int col, float size);
    static std::unique_ptr<Gem> createSpecialGem(GemType type, GemColor color, int row, int col, float size);
};