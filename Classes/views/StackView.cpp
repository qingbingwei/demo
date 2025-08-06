#include "StackView.h"
#include "CardView.h"
#include <algorithm>
#include "controllers/GameController.h"
USING_NS_CC;

StackView* StackView::create() {
    StackView* ret = new (std::nothrow) StackView();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool StackView::init() {
    if (!Node::init()) return false;
    this->setContentSize(Size(STACK_WIDTH, STACK_HEIGHT));
    return true;
}

void StackView::addCard(CardView* card) {
    if (!card) return;
    card->retain();
    _cards.push_back(card);
    this->addChild(card);
    
    // 设置点击回调，所有卡牌都可以响应点击
    card->setOnClickCallback([this](int cardId) {
        CCLOG("StackView: Card clicked, id=%d", cardId);
        if (_onCardClickCallback) {
            _onCardClickCallback(cardId);
        }
    });
    
    // 判断是否是覆盖操作：如果卡片已经有了位置，则不重新布局
    if (card->getPosition().x == 0 && card->getPosition().y == 0) {
        layoutCards();
        CCLOG("StackView: Added card with layout, id=%d, total cards=%zu", 
              card->getCardId(), _cards.size());
    } else {
        CCLOG("StackView: Added card without layout (overlay), id=%d, pos=(%.1f, %.1f), total cards=%zu", 
              card->getCardId(), card->getPosition().x, card->getPosition().y, _cards.size());
    }
    
    card->release();
}

void StackView::removeCard(CardView* card) {
    if (!card) return;
    auto it = std::find(_cards.begin(), _cards.end(), card);
    if (it != _cards.end()) {
        card->retain();
        _cards.erase(it);
        card->removeFromParentAndCleanup(false);
        // 重要：对于撤销操作，我们可能不想立即重新布局
        // 只在正常移除时才布局
        layoutCards();
        CCLOG("StackView: Removed card id=%d", card->getCardId());
        card->release();
    }
}

void StackView::setOnCardClickCallback(const std::function<void(int)>& callback) {
    _onCardClickCallback = callback;
    // 重新设置所有卡牌的点击回调
    for (auto card : _cards) {
        card->setOnClickCallback([this](int cardId) {
            CCLOG("StackView: Card clicked via callback, id=%d", cardId);
            if (_onCardClickCallback) {
                _onCardClickCallback(cardId);
            }
        });
    }
}

void StackView::layoutCards() {
    const float cardSpacing = 25.0f; // 卡牌间距
    const float startX = 0.0f; // 起始X位置
    const float y = 0.0f; // Y位置
    
    for (size_t i = 0; i < _cards.size(); ++i) {
        float x = startX + static_cast<float>(i) * cardSpacing; // 每张卡牌向右偏移
        _cards[i]->setPosition(Vec2(x, y));
        _cards[i]->setLocalZOrder(static_cast<int>(i)); // 确保后面的卡牌在上层
        _cards[i]->setVisible(true); // 所有卡牌都可见
        
        CCLOG("StackView: Laid out card id=%d at pos=(%.1f, %.1f), zOrder=%d", 
              _cards[i]->getCardId(), x, y, static_cast<int>(i));
    }
}

void StackView::moveCardToTop(CardView* cardView) {
    auto it = std::find(_cards.begin(), _cards.end(), cardView);
    if (it != _cards.end()) {
        // 将卡牌移动到末尾（顶部）
        CardView* card = *it;
        _cards.erase(it);
        _cards.push_back(card);
        layoutCards();
        CCLOG("StackView: Moved card id=%d to top", cardView->getCardId());
    }
}

// 保存卡牌状态
void StackView::saveCardState(int cardId) {
    CardView* card = findCardById(cardId);
    if (card) {
        StackCardRestoreInfo info;
        info.cardId = cardId;
        info.position = card->getPosition();
        info.zOrder = card->getLocalZOrder();
        info.visible = card->isVisible();
        
        _cardStates[cardId] = info;
        
        CCLOG("StackView: Saved state for card id=%d, pos=(%.1f,%.1f), zOrder=%d, visible=%d", 
              cardId, info.position.x, info.position.y, info.zOrder, info.visible ? 1 : 0);
    }
}

// 恢复卡牌状态
void StackView::restoreCardState(int cardId) {
    auto it = _cardStates.find(cardId);
    if (it != _cardStates.end()) {
        CardView* card = findCardById(cardId);
        if (card) {
            const StackCardRestoreInfo& info = it->second;
            card->setPosition(info.position);
            card->setLocalZOrder(info.zOrder);
            card->setVisible(info.visible);
            
            CCLOG("StackView: Restored state for card id=%d, pos=(%.1f,%.1f), zOrder=%d, visible=%d", 
                  cardId, info.position.x, info.position.y, info.zOrder, info.visible ? 1 : 0);
        }
    }
}

// 通过ID查找卡片
CardView* StackView::findCardById(int cardId) const {
    for (auto card : _cards) {
        if (card->getCardId() == cardId) {
            return card;
        }
    }
    return nullptr;
}