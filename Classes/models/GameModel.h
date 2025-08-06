#pragma once
#include <vector>
#include "CardModel.h"

struct GameModel {
    std::vector<CardModel> playfieldCards;
    std::vector<CardModel> stackCards;
    std::vector<CardModel> reserveCards;
    std::vector<CardModel> removedPlayfieldCards; // 记录移除的卡牌
    std::vector<CardModel> removedBaseCards;
    int topStackIndex = -1; // 当前底牌堆顶部索引
    int nextCardId = 0;     // 卡牌ID计数器

    void clear();
    int getNextCardId();
    void addCardToReserveStack(const CardModel& card);
    void addCardToPlayfield(const CardModel& card);
    void addCardToBaseStack(const CardModel& card);
    bool isCardInReserveStack(int cardId) const;
    bool isCardInBaseStack(int cardId) const;
    bool isCardInPlayfield(int cardId) const;
    bool canMoveReserveToBase(int cardId) const;
    void moveCardFromReserveToBase(int cardId);
    void moveCardFromBaseToReserve(int cardId);
    void moveCardToBaseTop(int cardId);
    void restoreBaseStackOrder(int cardId);
    CardModel getBaseStackTop() const;
    void removeCardFromPlayfield(int cardId);
    void removeCardFromBaseStack(int cardId);
    CardModel getLastRemovedPlayfieldCard() const;
    CardModel getLastRemovedBaseCard() const;
    CardModel getCardById(int cardId) const;
};