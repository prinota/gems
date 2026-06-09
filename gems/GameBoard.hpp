#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "Gem.hpp"

enum class BoardState {
    WaitingForInput,
    AnimatingSwap,
    RevertingSwap,
    CheckingMatches,
    MovingBonuses,
    FallingGems,
    SpawningNewGems
};

class GameBoard {
public:
    GameBoard(int rows, int cols, float gemSize);

    void draw(sf::RenderWindow& window);
    bool handleClick(sf::Vector2f mousePos);
    void update(float dt);

    void recolorRandomGems(int centerRow, int centerCol, GemColor color, int count);
    void destroyGemsInRadius(int centerRow, int centerCol, int radius);

private:
    int rows, cols;
    float gemSize;
    std::vector<std::vector<std::unique_ptr<Gem>>> gems;
    sf::RectangleShape background;

    int selectedRow = -1, selectedCol = -1;
    int lastSwappedRow1 = -1, lastSwappedCol1 = -1;
    int lastSwappedRow2 = -1, lastSwappedCol2 = -1;

    BoardState currentState;
    float stateTimer;

    Gem* getGem(int row, int col);
    bool hasSelection() const;
    void clearSelection();
    bool areAdjacent(int row1, int col1, int row2, int col2) const;
    bool isInRadius(int row1, int col1, int row2, int col2, int radius) const;
    bool isValidCell(int row, int col) const;
    void swapGems(int row1, int col1, int row2, int col2);

    void initializeBoard();
    sf::Vector2f getGemPosition(int row, int col) const;

    int getRandomInt(int min, int max) const;
    float getRandomFloat() const;

    std::vector<std::vector<bool>> findMatches();
    bool findAndMarkMatches();
    bool isAnimating() const;

    void convertToSpecialGems();
    std::pair<int, int> getRandomTarget(int sourceRow, int sourceCol, int radius) const;
    void moveBonusesToTargets();
    void activateSpecialGems();

    void removeMatchedGems();
    void applyGravity();
    void spawnNewGems();
    void processFalling(float dt);
};