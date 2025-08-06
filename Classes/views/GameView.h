#pragma once
#ifndef GAME_VIEW_H
#define GAME_VIEW_H

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "CardView.h"
#include "StackView.h"
#include "PlayfieldView.h"


class GameController; // 添加前向声明

class GameView : public cocos2d::Scene {
public:
    static cocos2d::Scene* createScene();
    virtual bool init();
    CREATE_FUNC(GameView);

    void setOnCardClickCallback(const std::function<void(int)>& callback);
    void setOnUndoClickCallback(const std::function<void()>& callback);
    void showUndoButton(bool visible);
    void addCardToPlayfield(CardView* cardView);
    void addCardToStack(CardView* cardView);
    void onReserveCardClicked(CardView* cardView);
    void onHandCardClicked(CardView* cardView);
    PlayfieldView* getPlayfieldView() { return _playfieldView; }
    StackView* getBaseStackView() { return _baseStackView; }
    StackView* getReserveStackView() { return _reserveStackView; }

private:
    PlayfieldView* _playfieldView;
    StackView* _baseStackView;
    StackView* _reserveStackView;
    cocos2d::ui::Button* _undoButton;
    std::function<void(int)> _onCardClickCallback;
    std::function<void()> _onUndoClickCallback;
    GameController* _controller; // 确保声明
};

#endif // GAME_VIEW_H