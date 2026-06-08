#pragma once
#include "Gem.hpp"
#include <vector>
#include <memory>

class GameBoard {
public:
    GameBoard(int rows, int cols, float gemSize);

    void draw(sf::RenderWindow& window);
    bool handleClick(sf::Vector2f mousePos);
    void update(float dt);
    bool isAnimating() const;

    void recolorRandomGems(int centerRow, int centerCol, GemColor color, int count);
    void destroyGemsInRadius(int centerRow, int centerCol, int radius);

private:
    int rows, cols;
    float gemSize;
    std::vector<std::vector<std::unique_ptr<Gem>>> gems;
    Gem* selectedGem;
    Gem* lastSwappedGem1;
    Gem* lastSwappedGem2;
    sf::RectangleShape background;

    enum class BoardState {
        WaitingForInput,
        AnimatingSwap,
        RevertingSwap,
        CheckingMatches,
        MovingBonuses,
        RemovingMatches,
        FallingGems,
        SpawningNewGems
    };

    BoardState currentState;
    float stateTimer;

    void initializeBoard();
    void swapGems(Gem& gem1, Gem& gem2);
    bool areAdjacent(const Gem& gem1, const Gem& gem2) const;
    sf::Vector2f getGemPosition(int row, int col) const;

    bool findAndMarkMatches();
    void moveBonusesToTargets();
    void activateSpecialGems();
    void convertToSpecialGems();
    void removeMatchedGems();
    void applyGravity();
    void spawnNewGems();
    void processFalling(float dt);

    std::vector<std::vector<bool>> findMatches();

    bool isInRadius(int row1, int col1, int row2, int col2, int radius) const;
    std::pair<int, int> getRandomTarget(int sourceRow, int sourceCol, int radius) const;

    int getRandomInt(int min, int max) const;
    float getRandomFloat() const;
};