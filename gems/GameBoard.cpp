#include "GameBoard.hpp"
#include <algorithm>
#include <cstdlib>
#include <ctime>

GameBoard::GameBoard(int rows, int cols, float gemSize)
    : rows(rows), cols(cols), gemSize(gemSize),
    currentState(BoardState::WaitingForInput), stateTimer(0.0f) {

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    float boardWidth = cols * gemSize;
    float boardHeight = rows * gemSize;

    background.setSize(sf::Vector2f(boardWidth, boardHeight));
    background.setFillColor(sf::Color(40, 40, 40));
    background.setOutlineThickness(2);
    background.setOutlineColor(sf::Color(100, 100, 100));

    initializeBoard();
}

// ¬спомогательные методы
Gem* GameBoard::getGem(int row, int col) {
    if (!isValidCell(row, col)) return nullptr;
    return gems[row][col].get();
}

bool GameBoard::hasSelection() const {
    return selectedRow != -1 && selectedCol != -1;
}

void GameBoard::clearSelection() {
    if (hasSelection()) {
        gems[selectedRow][selectedCol]->setSelected(false);
    }
    selectedRow = -1;
    selectedCol = -1;
}

bool GameBoard::isValidCell(int row, int col) const {
    return row >= 0 && row < rows && col >= 0 && col < cols;
}

bool GameBoard::areAdjacent(int row1, int col1, int row2, int col2) const {
    int rowDiff = std::abs(row1 - row2);
    int colDiff = std::abs(col1 - col2);
    return (rowDiff + colDiff) == 1;
}

int GameBoard::getRandomInt(int min, int max) const {
    return min + (std::rand() % (max - min + 1));
}

float GameBoard::getRandomFloat() const {
    return static_cast<float>(std::rand()) / RAND_MAX;
}

void GameBoard::initializeBoard() {
    gems.resize(rows);
    for (int row = 0; row < rows; ++row) {
        gems[row].resize(cols);
        for (int col = 0; col < cols; ++col) {
            auto gem = GemFactory::createRandomGem(row, col, gemSize);
            sf::Vector2f pos = getGemPosition(row, col);
            gem->setPosition(pos.x, pos.y);
            gems[row][col] = std::move(gem);
        }
    }

    while (true) {
        auto matched = findMatches();
        bool hasMatches = false;
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                if (matched[row][col]) {
                    hasMatches = true;
                    auto newGem = GemFactory::createRandomGem(row, col, gemSize);
                    sf::Vector2f pos = getGemPosition(row, col);
                    newGem->setPosition(pos.x, pos.y);
                    gems[row][col] = std::move(newGem);
                }
            }
        }
        if (!hasMatches) break;
    }
}

sf::Vector2f GameBoard::getGemPosition(int row, int col) const {
    float x = col * gemSize + gemSize / 2;
    float y = row * gemSize + gemSize / 2;
    return sf::Vector2f(x, y);
}

void GameBoard::draw(sf::RenderWindow& window) {
    window.draw(background);

    for (const auto& row : gems) {
        for (const auto& gem : row) {
            gem->draw(window);
        }
    }
}

bool GameBoard::handleClick(sf::Vector2f mousePos) {
    if (currentState != BoardState::WaitingForInput) return false;

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            if (gems[row][col]->contains(mousePos)) {
                if (!hasSelection()) {
                    gems[row][col]->setSelected(true);
                    selectedRow = row;
                    selectedCol = col;
                }
                else if (selectedRow == row && selectedCol == col) {
                    //повторный клик - сн€ть выделение
                    gems[row][col]->setSelected(false);
                    clearSelection();
                }
                else if (areAdjacent(selectedRow, selectedCol, row, col)) {
                    lastSwappedRow1 = selectedRow;
                    lastSwappedCol1 = selectedCol;
                    lastSwappedRow2 = row;
                    lastSwappedCol2 = col;

                    gems[selectedRow][selectedCol]->setSelected(false);
                    swapGems(selectedRow, selectedCol, row, col);
                    clearSelection();
                    currentState = BoardState::AnimatingSwap;
                }
                else {
                    gems[selectedRow][selectedCol]->setSelected(false);
                    gems[row][col]->setSelected(true);
                    selectedRow = row;
                    selectedCol = col;
                }
                return true;
            }
        }
    }
    return false;
}

void GameBoard::swapGems(int row1, int col1, int row2, int col2) {
    std::swap(gems[row1][col1], gems[row2][col2]);

    gems[row1][col1]->setRow(row1);
    gems[row1][col1]->setCol(col1);
    gems[row2][col2]->setRow(row2);
    gems[row2][col2]->setCol(col2);

    sf::Vector2f newPos1 = getGemPosition(row1, col1);
    sf::Vector2f newPos2 = getGemPosition(row2, col2);

    gems[row1][col1]->setTargetPosition(newPos1.x, newPos1.y);
    gems[row2][col2]->setTargetPosition(newPos2.x, newPos2.y);
}

void GameBoard::update(float dt) {
    stateTimer += dt;

    switch (currentState) {
    case BoardState::AnimatingSwap:
        processFalling(dt);
        if (!isAnimating()) {
            if (findAndMarkMatches()) {
                lastSwappedRow1 = -1;
                lastSwappedCol1 = -1;
                lastSwappedRow2 = -1;
                lastSwappedCol2 = -1;
                currentState = BoardState::CheckingMatches;
                stateTimer = 0.0f;
            }
            else {
                //обратный свап
                if (lastSwappedRow1 != -1 && lastSwappedRow2 != -1) {
                    swapGems(lastSwappedRow1, lastSwappedCol1,
                        lastSwappedRow2, lastSwappedCol2);
                    lastSwappedRow1 = -1;
                    lastSwappedCol1 = -1;
                    lastSwappedRow2 = -1;
                    lastSwappedCol2 = -1;
                    currentState = BoardState::RevertingSwap;
                }
                else {
                    currentState = BoardState::WaitingForInput;
                }
            }
        }
        break;

    case BoardState::RevertingSwap:
        processFalling(dt);
        if (!isAnimating()) {
            currentState = BoardState::WaitingForInput;
        }
        break;

    case BoardState::CheckingMatches:
        if (stateTimer >= 0.3f) {
            convertToSpecialGems();
            moveBonusesToTargets();
            currentState = BoardState::MovingBonuses;
            stateTimer = 0.0f;
        }
        break;

    case BoardState::MovingBonuses:
        processFalling(dt);
        if (!isAnimating()) {
            activateSpecialGems();
            removeMatchedGems();
            applyGravity();
            currentState = BoardState::FallingGems;
            stateTimer = 0.0f;
        }
        break;

    case BoardState::FallingGems:
        processFalling(dt);
        if (!isAnimating()) {
            spawnNewGems();
            currentState = BoardState::SpawningNewGems;
            stateTimer = 0.0f;
        }
        break;

    case BoardState::SpawningNewGems:
        processFalling(dt);
        if (!isAnimating()) {
            if (findAndMarkMatches()) {
                currentState = BoardState::CheckingMatches;
                stateTimer = 0.0f;
            }
            else {
                currentState = BoardState::WaitingForInput;
            }
        }
        break;

    default:
        break;
    }
}

bool GameBoard::isAnimating() const {
    for (const auto& row : gems) {
        for (const auto& gem : row) {
            if (gem->isAnimating()) {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::vector<bool>> GameBoard::findMatches() {
    std::vector<std::vector<bool>> matched(rows, std::vector<bool>(cols, false));
    std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            if (!gems[row][col]->isEmpty() && !visited[row][col]) {
                GemColor targetColor = gems[row][col]->getColor();
                std::vector<std::pair<int, int>> component;
                std::vector<std::pair<int, int>> queue;

                queue.push_back(std::make_pair(row, col));
                visited[row][col] = true;

                while (!queue.empty()) {
                    std::pair<int, int> current = queue.back();
                    queue.pop_back();
                    int r = current.first;
                    int c = current.second;
                    component.push_back(std::make_pair(r, c));

                    const int dr[] = { -1, 1, 0, 0 };
                    const int dc[] = { 0, 0, -1, 1 };

                    for (int i = 0; i < 4; ++i) {
                        int nr = r + dr[i];
                        int nc = c + dc[i];

                        if (isValidCell(nr, nc) &&
                            !visited[nr][nc] && !gems[nr][nc]->isEmpty() &&
                            gems[nr][nc]->getColor() == targetColor) {
                            visited[nr][nc] = true;
                            queue.push_back(std::make_pair(nr, nc));
                        }
                    }
                }

                if (component.size() >= 3) {
                    for (size_t i = 0; i < component.size(); ++i) {
                        matched[component[i].first][component[i].second] = true;
                    }
                }
            }
        }
    }

    return matched;
}

bool GameBoard::findAndMarkMatches() {
    auto matched = findMatches();
    bool hasMatches = false;

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            if (matched[row][col]) {
                hasMatches = true;
                gems[row][col]->setState(GemState::Matched);
            }
        }
    }

    return hasMatches;
}

void GameBoard::convertToSpecialGems() {
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            if (gems[row][col]->getState() == GemState::Matched &&
                !gems[row][col]->isSpecial()) {

                if (getRandomFloat() < 0.15f) {
                    GemType type = (getRandomInt(0, 1) == 0) ? GemType::Recolor : GemType::Bomb;
                    GemColor color = gems[row][col]->getColor();

                    auto specialGem = GemFactory::createSpecialGem(type, color, row, col, gemSize);
                    sf::Vector2f pos = getGemPosition(row, col);
                    specialGem->setPosition(pos.x, pos.y);
                    specialGem->setState(GemState::Matched);

                    gems[row][col] = std::move(specialGem);
                }
            }
        }
    }
}

std::pair<int, int> GameBoard::getRandomTarget(int sourceRow, int sourceCol, int radius) const {
    std::vector<std::pair<int, int>> targets;

    for (int r = sourceRow - radius; r <= sourceRow + radius; ++r) {
        for (int c = sourceCol - radius; c <= sourceCol + radius; ++c) {
            if (isValidCell(r, c)) {
                if (!(r == sourceRow && c == sourceCol)) {
                    if (!gems[r][c]->isEmpty() && !gems[r][c]->isSpecial()) {
                        targets.push_back(std::make_pair(r, c));
                    }
                }
            }
        }
    }

    if (targets.empty()) {
        return std::make_pair(sourceRow, sourceCol);
    }

    int index = getRandomInt(0, targets.size() - 1);
    return targets[index];
}

void GameBoard::moveBonusesToTargets() {
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            if (gems[row][col]->getState() == GemState::Matched &&
                gems[row][col]->isSpecial()) {

                std::pair<int, int> target = getRandomTarget(row, col, 3);
                int targetRow = target.first;
                int targetCol = target.second;

                if (targetRow != row || targetCol != col) {
                    auto specialGem = std::move(gems[row][col]);

                    gems[row][col] = std::move(gems[targetRow][targetCol]);
                    gems[row][col]->setRow(row);
                    gems[row][col]->setCol(col);
                    gems[row][col]->setState(GemState::Matched);

                    specialGem->setRow(targetRow);
                    specialGem->setCol(targetCol);
                    specialGem->setState(GemState::BonusMoving);

                    sf::Vector2f targetPos = getGemPosition(targetRow, targetCol);
                    specialGem->setTargetPosition(targetPos.x, targetPos.y);

                    gems[targetRow][targetCol] = std::move(specialGem);
                }
            }
        }
    }
}

void GameBoard::activateSpecialGems() {
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            if (gems[row][col]->isSpecial() &&
                (gems[row][col]->getState() == GemState::Matched ||
                    gems[row][col]->getState() == GemState::BonusMoving)) {
                gems[row][col]->activateEffect(*this);
                gems[row][col]->setState(GemState::Empty);
            }
        }
    }
}

void GameBoard::removeMatchedGems() {
    for (auto& row : gems) {
        for (auto& gem : row) {
            if (gem->getState() == GemState::Matched) {
                gem->setState(GemState::Empty);
            }
        }
    }
}

void GameBoard::applyGravity() {
    for (int col = 0; col < cols; ++col) {
        int emptyRow = rows - 1;

        for (int row = rows - 1; row >= 0; --row) {
            if (!gems[row][col]->isEmpty()) {
                if (row != emptyRow) {
                    std::swap(gems[row][col], gems[emptyRow][col]);

                    gems[emptyRow][col]->setRow(emptyRow);
                    gems[emptyRow][col]->setCol(col);
                    gems[emptyRow][col]->setState(GemState::Falling);

                    sf::Vector2f target = getGemPosition(emptyRow, col);
                    gems[emptyRow][col]->setTargetPosition(target.x, target.y);

                    gems[row][col]->setRow(row);
                    gems[row][col]->setCol(col);
                    gems[row][col]->setState(GemState::Empty);
                }
                emptyRow--;
            }
        }
    }
}

void GameBoard::spawnNewGems() {
    for (int col = 0; col < cols; ++col) {
        int emptyCount = 0;

        for (int row = 0; row < rows; ++row) {
            if (gems[row][col]->isEmpty()) {
                emptyCount++;

                auto newGem = GemFactory::createRandomGem(row, col, gemSize);
                float startY = -gemSize * (emptyCount);
                sf::Vector2f target = getGemPosition(row, col);

                newGem->setPosition(target.x, startY);
                newGem->setState(GemState::Falling);
                newGem->setTargetPosition(target.x, target.y);

                gems[row][col] = std::move(newGem);
            }
        }
    }
}

void GameBoard::processFalling(float dt) {
    for (auto& row : gems) {
        for (auto& gem : row) {
            if (gem->isAnimating()) {
                sf::Vector2f target = getGemPosition(gem->getRow(), gem->getCol());
                gem->animateTo(target.x, target.y, dt);
            }
        }
    }
}

bool GameBoard::isInRadius(int row1, int col1, int row2, int col2, int radius) const {
    int rowDiff = std::abs(row1 - row2);
    int colDiff = std::abs(col1 - col2);
    return rowDiff <= radius && colDiff <= radius;
}

void GameBoard::recolorRandomGems(int centerRow, int centerCol, GemColor color, int count) {
    std::vector<std::pair<int, int>> candidates;

    for (int r = centerRow - 3; r <= centerRow + 3; ++r) {
        for (int c = centerCol - 3; c <= centerCol + 3; ++c) {
            if (isValidCell(r, c)) {
                if (!(std::abs(r - centerRow) <= 1 && std::abs(c - centerCol) <= 1)) {
                    if (!gems[r][c]->isEmpty()) {
                        candidates.push_back(std::make_pair(r, c));
                    }
                }
            }
        }
    }

    for (size_t i = candidates.size() - 1; i > 0; --i) {
        size_t j = getRandomInt(0, i);
        std::swap(candidates[i], candidates[j]);
    }

    int applied = 0;
    for (size_t i = 0; i < candidates.size(); ++i) {
        if (applied >= count) break;
        gems[candidates[i].first][candidates[i].second]->setColor(color);
        applied++;
    }
}

void GameBoard::destroyGemsInRadius(int centerRow, int centerCol, int radius) {
    for (int r = centerRow - radius; r <= centerRow + radius; ++r) {
        for (int c = centerCol - radius; c <= centerCol + radius; ++c) {
            if (isValidCell(r, c)) {
                if (isInRadius(centerRow, centerCol, r, c, radius)) {
                    if (!gems[r][c]->isEmpty()) {
                        gems[r][c]->setState(GemState::Empty);
                    }
                }
            }
        }
    }
}