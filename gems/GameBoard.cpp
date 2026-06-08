#include "GameBoard.hpp"
#include <algorithm>
#include <cstdlib>
#include <ctime>

GameBoard::GameBoard(int rows, int cols, float gemSize)
    : rows(rows), cols(cols), gemSize(gemSize), selectedGem(nullptr),
    lastSwappedGem1(nullptr), lastSwappedGem2(nullptr),
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

    for (auto& row : gems) {
        for (auto& gem : row) {
            if (gem->contains(mousePos)) {
                if (selectedGem == nullptr) {
                    gem->setSelected(true);
                    selectedGem = gem.get();
                }
                else if (selectedGem == gem.get()) {
                    gem->setSelected(false);
                    selectedGem = nullptr;
                }
                else if (areAdjacent(*selectedGem, *gem)) {
                    lastSwappedGem1 = selectedGem;
                    lastSwappedGem2 = gem.get();

                    selectedGem->setSelected(false);
                    swapGems(*selectedGem, *gem);
                    selectedGem = nullptr;
                    currentState = BoardState::AnimatingSwap;
                }
                else {
                    selectedGem->setSelected(false);
                    gem->setSelected(true);
                    selectedGem = gem.get();
                }
                return true;
            }
        }
    }
    return false;
}

bool GameBoard::areAdjacent(const Gem& gem1, const Gem& gem2) const {
    int rowDiff = std::abs(gem1.getRow() - gem2.getRow());
    int colDiff = std::abs(gem1.getCol() - gem2.getCol());
    return (rowDiff + colDiff) == 1;
}

void GameBoard::swapGems(Gem& gem1, Gem& gem2) {
    int oldRow1 = gem1.getRow();
    int oldCol1 = gem1.getCol();
    int oldRow2 = gem2.getRow();
    int oldCol2 = gem2.getCol();

    std::swap(gems[oldRow1][oldCol1], gems[oldRow2][oldCol2]);

    gem1.setRow(oldRow2);
    gem1.setCol(oldCol2);
    gem2.setRow(oldRow1);
    gem2.setCol(oldCol1);

    sf::Vector2f newPos1 = getGemPosition(oldRow2, oldCol2);
    sf::Vector2f newPos2 = getGemPosition(oldRow1, oldCol1);

    gem1.setTargetPosition(newPos1.x, newPos1.y);
    gem2.setTargetPosition(newPos2.x, newPos2.y);
}

void GameBoard::update(float dt) {
    stateTimer += dt;

    switch (currentState) {
    case BoardState::AnimatingSwap:
        processFalling(dt);
        if (!isAnimating()) {
            if (findAndMarkMatches()) {
                lastSwappedGem1 = nullptr;
                lastSwappedGem2 = nullptr;
                currentState = BoardState::CheckingMatches;
                stateTimer = 0.0f;
            }
            else {
                // Обратный свап
                if (lastSwappedGem1 && lastSwappedGem2) {
                    swapGems(*lastSwappedGem1, *lastSwappedGem2);
                    lastSwappedGem1 = nullptr;
                    lastSwappedGem2 = nullptr;
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

                        if (nr >= 0 && nr < rows && nc >= 0 && nc < cols &&
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
            if (r >= 0 && r < rows && c >= 0 && c < cols) {
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
            if (r >= 0 && r < rows && c >= 0 && c < cols) {
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
            if (r >= 0 && r < rows && c >= 0 && c < cols) {
                if (isInRadius(centerRow, centerCol, r, c, radius)) {
                    if (!gems[r][c]->isEmpty()) {
                        gems[r][c]->setState(GemState::Empty);
                    }
                }
            }
        }
    }
}