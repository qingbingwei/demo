#pragma once
#include "cocos2d.h"
#include <vector>
#include <unordered_map>
#include "CardView.h"

// 用于保存卡牌状态的结构体
struct StackCardRestoreInfo {
    int cardId;
    cocos2d::Vec2 position;
    int zOrder;
    bool visible;
};

class StackView : public cocos2d::Node {
public:
    static StackView* create();
    virtual bool init();
    void addCard(CardView* card);
    void removeCard(CardView* card);
    void setOnCardClickCallback(const std::function<void(int)>& callback);
    void layoutCards();
    void moveCardToTop(CardView* cardView);
    
    // 获取顶部卡片 - 默认是最后添加的卡片
    CardView* getTopCard() const {
        return _cards.empty() ? nullptr : _cards.back();
    }
    
    // 获取所有卡片
    std::vector<CardView*>& getCards() { return _cards; }
    
    // 保存和恢复卡牌状态的方法
    void saveCardState(int cardId);
    void restoreCardState(int cardId);
    CardView* findCardById(int cardId) const;
    
    static const int STACK_WIDTH = 110;
    static const int STACK_HEIGHT = 150;
    
private:
    std::vector<CardView*> _cards;
    std::function<void(int)> _onCardClickCallback;
    std::unordered_map<int, StackCardRestoreInfo> _cardStates; // 保存卡牌状态
};
