#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "views/CardView.h"
#include "views/StackView.h"
#include "views/PlayfieldView.h"
#include "views/GameView.h"
#include "configs/loaders/LevelConfigLoader.h"
#include "managers/UndoManager.h"
#include <vector>
#include <stack>
#include <functional>

struct CardModel {
    int id;
    int face;
    int suit;
    bool isFaceUp;
    bool isRemoved;
    float posX;
    float posY;
};

struct MoveRecord {
    int cardId;
    bool fromPlayfield;
    bool toBaseStack;
};

class GameModel {
public:
    void clear();
    int getNextCardId();
    void addCardToReserveStack(const CardModel& card);
    void addCardToPlayfield(const CardModel& card);
    void addCardToBaseStack(const CardModel& card);
    void moveCardFromBaseToReserve(int cardId);
    void restoreBaseStackOrder(int cardId);
    CardModel getLastRemovedPlayfieldCard();
    CardModel getLastRemovedBaseCard();
    CardModel getCardById(int cardId);

private:
    std::vector<CardModel> cards;
    std::vector<CardModel> baseStack;
    std::vector<CardModel> reserveStack;
    std::vector<CardModel> playfield;
    int nextCardId = 0;
};

class GameController {
public:
    GameController(GameView* view);
    void startGame(const LevelConfig& config);
    void onCardClicked(int cardId);
    void onUndoClicked();
    bool canMatch(int playfieldCardId, int stackTopCardId);
    CardView* findCardViewById(int cardId, PlayfieldView* view);
    CardView* findCardViewById(int cardId, StackView* view);

private:
    GameView* _gameView;
    GameModel _gameModel;
    UndoManager _undoManager;
    std::stack<MoveRecord> _moveHistory;
    std::vector<CardView*> _baseStackSnapshot; // 手牌区状态快照用于撤销

    void updateView();
    bool canMatchCards(int face1, int face2); // 支持A-K循环匹配的方法
    void restoreBaseStackOrder(StackView* baseStackView, const std::vector<CardView*>& snapshot); // 恢复手牌区顺序

    // 新增：查找与指定卡牌可匹配的所有桌面牌
    std::vector<CardView*> getMatchablePlayfieldCards(CardView* cardToMatch);
    // 新增：获取桌面上可点击的卡牌（可见且未被覆盖）
    std::vector<CardView*> getClickablePlayfieldCards();
};

#endif // GAME_CONTROLLER_H