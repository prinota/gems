#include "GameBoard.hpp"
#include <algorithm>
#include <set>

GameBoard::GameBoard(int rows, int cols, float gemSize)
    : rows(rows), cols(cols), gemSize(gemSize), selectedGem(nullptr),
    currentState(BoardState::WaitingForInput), stateTimer(0.0f) {

    float boardWidth = cols * gemSize;
    float boardHeight = rows * gemSize;

    background.setSize(sf::Vector2f(boardWidth, boardHeight));
    background.setFillColor(sf::Color(40, 40, 40));
    background.setOutlineThickness(2);
    background.setOutlineColor(sf::Color(100, 100, 100));

    std::random_device rd;
    rng.seed(rd());

    initializeBoard();
}

void GameBoard::initializeBoard() {
    gems.resize(rows);
    for (int row = 0; row < rows; ++row) {
        gems[row].resize(cols);
        for (int col = 0; col < cols; ++col) {
            auto color = getRandomColor();
            auto gem = std::make_unique<Gem>(color, row, col, gemSize);
            sf::Vector2f pos = getGemPosition(row, col);
            gem->setPosition(pos.x, pos.y);
            gems[row][col] = std::move(gem);
        }
    }

    // Remove initial matches
    while (true) {
        auto matched = findMatches();
        bool hasMatches = false;
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                if (matched[row][col]) {
                    hasMatches = true;
                    gems[row][col]->setColor(getRandomColor());
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

GemColor GameBoard::getRandomColor() {
    std::uniform_int_distribution<int> dist(0, static_cast<int>(GemColor::Count) - 1);
    return static_cast<GemColor>(dist(rng));
}

void GameBoard::draw(sf::RenderWindow& window) {
    window.draw(background);

    for (const auto& row : gems) {
        for (const auto& gem : row) {
            gem->draw(window);
        }
    }

    // Бонусы рисуем поверх гемов
    for (const auto& bonus : activeBonuses) {
        bonus->draw(window);
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

    // Обновляем активные бонусы
    for (auto& bonus : activeBonuses) {
        bonus->update(dt);
    }

    // Удаляем завершённые бонусы
    activeBonuses.erase(
        std::remove_if(activeBonuses.begin(), activeBonuses.end(),
            [](const std::unique_ptr<Bonus>& b) { return b->isFinished(); }),
        activeBonuses.end()
    );

    switch (currentState) {
    case BoardState::AnimatingSwap:
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

    case BoardState::CheckingMatches:
        if (stateTimer >= 0.3f) {
            spawnBonuses();
            removeMatchedGems();
            currentState = BoardState::ProcessingBonuses;
            stateTimer = 0.0f;
        }
        break;

    case BoardState::ProcessingBonuses:
        // Ждём пока все бонусы будут готовы
        if (!activeBonuses.empty()) {
            bool allReady = true;
            for (const auto& bonus : activeBonuses) {
                if (!bonus->isReadyToApply()) {
                    allReady = false;
                    break;
                }
            }

            if (allReady) {
                applyBonusEffects();
                activeBonuses.clear();
                applyGravity();
                currentState = BoardState::FallingGems;
                stateTimer = 0.0f;
            }
        }
        else {
            // Если бонусов нет, сразу падаем
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

// Вспомогательные функции для бонусов
bool GameBoard::isInRadius(int row1, int col1, int row2, int col2, int radius) const {
    int rowDiff = std::abs(row1 - row2);
    int colDiff = std::abs(col1 - col2);
    return rowDiff <= radius && colDiff <= radius;
}

std::vector<std::pair<int, int>> GameBoard::getNonAdjacentInRadius(int centerRow, int centerCol, int radius, int count) {
    std::vector<std::pair<int, int>> candidates;

    // Собираем все позиции в радиусе
    for (int r = centerRow - radius; r <= centerRow + radius; ++r) {
        for (int c = centerCol - radius; c <= centerCol + radius; ++c) {
            if (r >= 0 && r < rows && c >= 0 && c < cols) {
                // Проверяем, что это не центр и не сосед
                if (!(std::abs(r - centerRow) <= 1 && std::abs(c - centerCol) <= 1)) {
                    candidates.push_back({ r, c });
                }
            }
        }
    }

    // Перемешиваем и выбираем нужное количество
    std::shuffle(candidates.begin(), candidates.end(), rng);

    std::vector<std::pair<int, int>> result;
    for (int i = 0; i < std::min(count, static_cast<int>(candidates.size())); ++i) {
        result.push_back(candidates[i]);
    }

    return result;
}

std::vector<std::pair<int, int>> GameBoard::getRandomPositions(int count, std::pair<int, int> exclude) {
    std::vector<std::pair<int, int>> positions;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (exclude.first != r || exclude.second != c) {
                positions.push_back({ r, c });
            }
        }
    }

    std::shuffle(positions.begin(), positions.end(), rng);

    std::vector<std::pair<int, int>> result;
    for (int i = 0; i < std::min(count, static_cast<int>(positions.size())); ++i) {
        result.push_back(positions[i]);
    }

    return result;
}

void GameBoard::spawnBonuses() {
    std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
    std::uniform_int_distribution<int> typeDist(0, 1);

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            if (gems[row][col]->getState() == GemState::Matched) {
                // 40% шанс на создание бонуса
                if (chanceDist(rng) < 0.4f) {
                    BonusType type = (typeDist(rng) == 0) ? BonusType::Recolor : BonusType::Bomb;

                    // Выбираем цель в радиусе 3
                    std::vector<std::pair<int, int>> targets;
                    for (int r = row - 3; r <= row + 3; ++r) {
                        for (int c = col - 3; c <= col + 3; ++c) {
                            if (r >= 0 && r < rows && c >= 0 && c < cols) {
                                targets.push_back({ r, c });
                            }
                        }
                    }

                    if (!targets.empty()) {
                        std::uniform_int_distribution<int> targetDist(0, targets.size() - 1);
                        auto target = targets[targetDist(rng)];

                        auto bonus = std::make_unique<Bonus>(
                            type, row, col, target.first, target.second,
                            gems[row][col]->getColor(), gemSize
                        );
                        activeBonuses.push_back(std::move(bonus));
                    }
                }
            }
        }
    }
}

void GameBoard::applyBonusEffects() {
    for (const auto& bonus : activeBonuses) {
        if (!bonus->isReadyToApply()) continue;  

        int targetRow = bonus->getTargetRow();
        int targetCol = bonus->getTargetCol();

        // Проверяем, что цель в пределах поля
        if (targetRow < 0 || targetRow >= rows || targetCol < 0 || targetCol >= cols) {
            continue;
        }

        if (bonus->getType() == BonusType::Recolor) {
            applyRecolorBonus(targetRow, targetCol, bonus->getSourceColor());
        }
        else {
            applyBombBonus(targetRow, targetCol);
        }
    }
}

void GameBoard::applyRecolorBonus(int targetRow, int targetCol, GemColor sourceColor) {
    // Перекрашиваем целевой квадрат
    if (!gems[targetRow][targetCol]->isEmpty()) {
        gems[targetRow][targetCol]->setColor(sourceColor);
    }

    // Выбираем 2 несоседа в радиусе 3 и перекрашиваем их
    auto nonAdjacent = getNonAdjacentInRadius(targetRow, targetCol, 3, 2);

    for (const auto& pos : nonAdjacent) {
        if (!gems[pos.first][pos.second]->isEmpty()) {
            gems[pos.first][pos.second]->setColor(sourceColor);
        }
    }
}

void GameBoard::applyBombBonus(int targetRow, int targetCol) {
    // Уничтожаем целевой квадрат
    if (!gems[targetRow][targetCol]->isEmpty()) {
        gems[targetRow][targetCol]->setState(GemState::Empty);
    }

    // Выбираем 4 случайных квадрата (плюс целевой = 5)
    auto randomPositions = getRandomPositions(4, { targetRow, targetCol });

    for (const auto& pos : randomPositions) {
        if (!gems[pos.first][pos.second]->isEmpty()) {
            gems[pos.first][pos.second]->setState(GemState::Empty);
        }
    }
}

// Остальные методы остаются без изменений
std::vector<std::vector<bool>> GameBoard::findMatches() {
    std::vector<std::vector<bool>> matched(rows, std::vector<bool>(cols, false));
    checkHorizontalMatches(matched);
    checkVerticalMatches(matched);
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

                auto newGem = std::make_unique<Gem>(getRandomColor(), row, col, gemSize);
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


bool GameBoard::checkHorizontalMatches(std::vector<std::vector<bool>>& matched) {
    bool found = false;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols - 2; ++col) {
            if (gems[row][col]->isEmpty()) continue;

            GemColor color = gems[row][col]->getColor();
            int matchLen = 1;

            while (col + matchLen < cols &&
                !gems[row][col + matchLen]->isEmpty() &&
                gems[row][col + matchLen]->getColor() == color) {
                matchLen++;
            }

            if (matchLen >= 3) {
                for (int c = col; c < col + matchLen; ++c) {
                    matched[row][c] = true;
                }
                found = true;
            }
            col += matchLen - 1;
        }
    }
    return found;
}

bool GameBoard::checkVerticalMatches(std::vector<std::vector<bool>>& matched) {
    bool found = false;
    for (int col = 0; col < cols; ++col) {
        for (int row = 0; row < rows - 2; ++row) {
            if (gems[row][col]->isEmpty()) continue;

            GemColor color = gems[row][col]->getColor();
            int matchLen = 1;

            while (row + matchLen < rows &&
                !gems[row + matchLen][col]->isEmpty() &&
                gems[row + matchLen][col]->getColor() == color) {
                matchLen++;
            }

            if (matchLen >= 3) {
                for (int r = row; r < row + matchLen; ++r) {
                    matched[r][col] = true;
                }
                found = true;
            }
            row += matchLen - 1;
        }
    }
    return found;
}