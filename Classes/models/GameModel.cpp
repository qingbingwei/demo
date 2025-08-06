#include "controllers/GameController.h"
#include <algorithm>

void GameModel::clear() {
    cards.clear();
    baseStack.clear();
    reserveStack.clear();
    playfield.clear();
    nextCardId = 0;
}

int GameModel::getNextCardId() {
    return nextCardId++;
}

void GameModel::addCardToReserveStack(const CardModel& card) {
    reserveStack.push_back(card);
}

void GameModel::addCardToPlayfield(const CardModel& card) {
    playfield.push_back(card);
}

void GameModel::addCardToBaseStack(const CardModel& card) {
    baseStack.push_back(card);
}

void GameModel::moveCardFromBaseToReserve(int cardId) {
    auto it = std::find_if(baseStack.begin(), baseStack.end(), [cardId](const CardModel& c) { return c.id == cardId; });
    if (it != baseStack.end()) {
        reserveStack.push_back(*it);
        baseStack.erase(it);
    }
}

void GameModel::restoreBaseStackOrder(int /*cardId*/) {
    // 实现顺序恢复逻辑（占位）
}

CardModel GameModel::getLastRemovedPlayfieldCard() {
    for (auto it = playfield.rbegin(); it != playfield.rend(); ++it) {
        if (it->isRemoved) return *it;
    }
    return CardModel{-1, 0, 0, false, false, 0, 0};
}

CardModel GameModel::getLastRemovedBaseCard() {
    for (auto it = baseStack.rbegin(); it != baseStack.rend(); ++it) {
        if (it->isRemoved) return *it;
    }
    return CardModel{-1, 0, 0, false, false, 0, 0};
}

CardModel GameModel::getCardById(int cardId) {
    for (const auto& card : cards) {
        if (card.id == cardId) return card;
    }
    return CardModel{-1, 0, 0, false, false, 0, 0};
}