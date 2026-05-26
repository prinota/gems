#pragma once
#include "Gem.hpp"
#include "Bonus.hpp"
#include <vector>
#include <memory>

class GameBoard {
public:
    GameBoard(int rows, int cols, float gemSize);

    void draw(sf::RenderWindow& window);
    bool handleClick(sf::Vector2f mousePos);
    void update(float dt);
    bool isAnimating() const;

private:
    int rows, cols;
    float gemSize;
    std::vector<std::vector<std::unique_ptr<Gem>>> gems;
    Gem* selectedGem;
    sf::RectangleShape background;
    std::vector<std::unique_ptr<Bonus>> activeBonuses;

    enum class BoardState {
        WaitingForInput,
        AnimatingSwap,
        CheckingMatches,
        RemovingMatches,
        ProcessingBonuses,
        FallingGems,
        SpawningNewGems
    };

    BoardState currentState;
    float stateTimer;

    void initializeBoard();
    void swapGems(Gem& gem1, Gem& gem2);
    bool areAdjacent(const Gem& gem1, const Gem& gem2) const;
    sf::Vector2f getGemPosition(int row, int col) const;
    GemColor getRandomColor();

    bool findAndMarkMatches();
    void removeMatchedGems();
    void spawnBonuses();
    void applyBonusEffects();
    void applyRecolorBonus(int targetRow, int targetCol, GemColor sourceColor);
    void applyBombBonus(int targetRow, int targetCol);
    void applyGravity();
    void spawnNewGems();
    void processFalling(float dt);

    bool isInRadius(int row1, int col1, int row2, int col2, int radius) const;
    std::vector<std::pair<int, int>> getNonAdjacentInRadius(int centerRow, int centerCol, int radius, int count);
    std::vector<std::pair<int, int>> getRandomPositions(int count, std::pair<int, int> exclude = { -1, -1 });

    std::vector<std::vector<bool>> findMatches();
    bool checkHorizontalMatches(std::vector<std::vector<bool>>& matched);
    bool checkVerticalMatches(std::vector<std::vector<bool>>& matched);

    std::mt19937 rng;
};