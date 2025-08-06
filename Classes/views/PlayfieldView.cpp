#include "PlayfieldView.h"
#include "CardView.h"
#include <algorithm>
#include <cmath>

USING_NS_CC;

PlayfieldView* PlayfieldView::create() {
    PlayfieldView* ret = new (std::nothrow) PlayfieldView();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

static const int PLAYFIELD_WIDTH = 1080;
static const int PLAYFIELD_HEIGHT = 1024;

bool PlayfieldView::init() {
    if (!Node::init()) return false;
    this->setContentSize(Size(PLAYFIELD_WIDTH, PLAYFIELD_HEIGHT));
    return true;
}

void PlayfieldView::addCard(CardView* cardView) {
    if (!cardView) return;
    
    // 保存当前位置
    Vec2 currentPos = cardView->getPosition();
    bool hasValidPosition = (currentPos.x != 0.0f || currentPos.y != 0.0f);
    
    // 保存是否可见和当前z-order
    bool wasVisible = cardView->isVisible();
    int originalZOrder = cardView->getLocalZOrder();
    
    _cards.push_back(cardView);
    this->addChild(cardView);
    
    // 确保z-order值正确 - 先加入的卡牌应该有较低的z-order
    if (originalZOrder >= 0) {
        // 如果有预设z-order值，使用它
        cardView->setLocalZOrder(originalZOrder);
        CCLOG("PlayfieldView: Using provided z-order: %d for card id=%d", 
              originalZOrder, cardView->getCardId());
    } else {
        // 如果没有预设值，则基于已有卡牌分配一个新值
        // 找到最高的z-order并加1
        int maxZOrder = -1;
        for (auto card : _cards) {
            if (card != cardView && card->getLocalZOrder() > maxZOrder) {
                maxZOrder = card->getLocalZOrder();
            }
        }
        
        // 如果没有找到其他卡牌，或所有卡牌都没有z-order，从0开始
        if (maxZOrder < 0) {
            maxZOrder = -1; // 这样 +1 后会等于0
        }
        
        cardView->setLocalZOrder(maxZOrder + 1);
        CCLOG("PlayfieldView: Assigned new z-order: %d for card id=%d", 
              maxZOrder + 1, cardView->getCardId());
    }
    
    // 确保卡片可见状态正确
    if (!wasVisible || cardView->getOpacity() == 0) {
        cardView->setVisible(false);
        cardView->setOpacity(0);
        CCLOG("PlayfieldView: Card id=%d set to invisible", cardView->getCardId());
    } else {
        cardView->setVisible(true);
        cardView->setOpacity(255); // 确保完全不透明
        CCLOG("PlayfieldView: Card id=%d set to visible", cardView->getCardId());
    }
    
    // 关键：如果卡牌已有有效位置，保持不变
    if (hasValidPosition) {
        cardView->setPosition(currentPos);
        CCLOG("PlayfieldView: Added card with preserved position, id=%d, pos=(%.1f, %.1f), zOrder=%d, visible=%d", 
              cardView->getCardId(), currentPos.x, currentPos.y, cardView->getLocalZOrder(), cardView->isVisible());
    } else {
        CCLOG("PlayfieldView: Added card with default position, id=%d, pos=(%.1f, %.1f), zOrder=%d, visible=%d", 
              cardView->getCardId(), cardView->getPosition().x, cardView->getPosition().y, 
              cardView->getLocalZOrder(), cardView->isVisible());
    }
    
    // 设置点击回调，包含覆盖检测 - 任何被覆盖的卡牌都不能被点击
    cardView->setOnClickCallback([this, cardView](int cardId) {
        // 检查卡牌是否被其他卡牌覆盖 - 无论重叠程度如何，被覆盖的卡牌不响应点击
        if (isCardCovered(cardView)) {
            CCLOG("PlayfieldView: Card id=%d is covered by other cards, click ignored", cardId);
            return;
        }
        
        CCLOG("PlayfieldView: Card clicked, id=%d", cardId);
        if (_onCardClickCallback) {
            _onCardClickCallback(cardId);
        } else {
            CCLOG("PlayfieldView: _onCardClickCallback is null for card id=%d", cardId);
        }
    });
}

void PlayfieldView::removeCard(CardView* cardView) {
    if (!cardView) return;
    
    this->removeChild(cardView);
    _cards.erase(std::remove(_cards.begin(), _cards.end(), cardView), _cards.end());
    
    // 重要：不调用layoutCards，以保持其他卡牌的原始位置
    // 只在特定情况下才重新布局
    // layoutCards();  // 注释掉这行
    
    CCLOG("PlayfieldView: Removed card id=%d, remaining cards: %zu", cardView->getCardId(), _cards.size());
}

void PlayfieldView::setOnCardClickCallback(const std::function<void(int)>& callback) {
    _onCardClickCallback = callback;
    CCLOG("PlayfieldView: Set card click callback, callback=%p", _onCardClickCallback ? 1 : 0);
    
    // 重新设置所有卡牌的点击回调，包含覆盖检测
    for (auto card : _cards) {
        card->setOnClickCallback([this, card](int cardId) {
            // 检查卡牌是否被其他卡牌覆盖 - 无论重叠程度如何，被覆盖的卡牌不响应点击
            if (isCardCovered(card)) {
                CCLOG("PlayfieldView: Card id=%d is covered by other cards, click ignored", cardId);
                return;
            }
            
            CCLOG("PlayfieldView: Card clicked, id=%d", cardId);
            if (_onCardClickCallback) {
                _onCardClickCallback(cardId);
            } else {
                CCLOG("PlayfieldView: _onCardClickCallback is null for card id=%d", cardId);
            }
        });
    }
}

// 检查卡牌是否被其他卡牌覆盖 - 更严格的实现
bool PlayfieldView::isCardCovered(CardView* targetCard) const {
    // 空指针检查
    if (!targetCard) {
        CCLOG("PlayfieldView: isCardCovered called with nullptr");
        return true; // 安全起见，视为被覆盖
    }
    
    // 基础可见性检查
    if (!targetCard->isVisible() || targetCard->getOpacity() == 0) {
        CCLOG("PlayfieldView: Card id=%d is not visible or transparent, considered covered", 
              targetCard->getCardId());
        return true;
    }
    
    // 父节点检查 - 确保卡牌是此PlayfieldView的子节点
    if (targetCard->getParent() != this) {
        CCLOG("PlayfieldView: Card id=%d's parent is not this PlayfieldView, cannot accurately check coverage", 
              targetCard->getCardId());
        return true; // 更严格：如果无法确定，视为被覆盖
    }
    
    Vec2 targetPos = targetCard->getPosition();
    int targetZOrder = targetCard->getLocalZOrder();
    
    // 获取目标卡牌的尺寸
    Size targetSize = targetCard->getContentSize();
    if (targetSize.width <= 0 || targetSize.height <= 0) {
        // 使用默认卡牌尺寸
        targetSize = Size(150.0f, 210.0f); // 使用更接近实际卡牌的尺寸
    }
    
    // 添加一个小的安全边距，使覆盖检测更严格
    const float SAFETY_MARGIN = 5.0f;
    targetSize.width += SAFETY_MARGIN * 2;
    targetSize.height += SAFETY_MARGIN * 2;
    
    CCLOG("PlayfieldView: Checking if card id=%d at pos=(%.1f,%.1f) with zOrder=%d is covered", 
          targetCard->getCardId(), targetPos.x, targetPos.y, targetZOrder);
    
    // 遍历所有卡牌，检查是否有重叠
    for (auto otherCard : _cards) {
        // 跳过目标卡牌自身和不可见/透明的卡牌
        if (otherCard == targetCard || !otherCard->isVisible() || otherCard->getOpacity() == 0) 
            continue;
        
        // 获取比较卡牌的信息
        Vec2 otherPos = otherCard->getPosition();
        int otherZOrder = otherCard->getLocalZOrder();
        
        // 只检查Z轴顺序更高的卡牌（可能覆盖目标卡牌的卡牌）
        if (otherZOrder <= targetZOrder) {
            CCLOG("PlayfieldView: Card id=%d (zOrder=%d) has lower/equal zOrder than target (zOrder=%d), not covering",
                  otherCard->getCardId(), otherZOrder, targetZOrder);
            continue;
        }
        
        // 获取比较卡牌的尺寸
        Size otherSize = otherCard->getContentSize();
        if (otherSize.width <= 0 || otherSize.height <= 0) {
            otherSize = Size(150.0f, 210.0f); // 使用更接近实际卡牌的尺寸
        }
        
        // 计算两个卡牌的碰撞盒
        float left1 = targetPos.x - (targetSize.width / 2);
        float right1 = targetPos.x + (targetSize.width / 2);
        float bottom1 = targetPos.y - (targetSize.height / 2);
        float top1 = targetPos.y + (targetSize.height / 2);
        
        float left2 = otherPos.x - (otherSize.width / 2);
        float right2 = otherPos.x + (otherSize.width / 2);
        float bottom2 = otherPos.y - (otherSize.height / 2);
        float top2 = otherPos.y + (otherSize.height / 2);
        
        // 计算重叠区域
        float overlapLeft = std::max(left1, left2);
        float overlapRight = std::min(right1, right2);
        float overlapBottom = std::max(bottom1, bottom2);
        float overlapTop = std::min(top1, top2);
        
        // 检查是否有任何重叠
        if (overlapLeft < overlapRight && overlapBottom < overlapTop) {
            // 计算重叠区域和覆盖百分比，用于调试
            float overlapWidth = overlapRight - overlapLeft;
            float overlapHeight = overlapTop - overlapBottom;
            float overlapArea = overlapWidth * overlapHeight;
            float targetArea = targetSize.width * targetSize.height;
            float coveragePercent = (overlapArea / targetArea) * 100.0f;
            
            CCLOG("PlayfieldView: Card id=%d is overlapped by card id=%d (%.2f%% coverage)", 
                  targetCard->getCardId(), otherCard->getCardId(), coveragePercent);
            
            // 无论覆盖比例多少，只要有重叠就算作被覆盖
            return true;
        }
    }
    
    CCLOG("PlayfieldView: Card id=%d is not covered by any other card", targetCard->getCardId());
    return false;
}

void PlayfieldView::layoutCards(LayoutType type) {
    _currentLayout = type;
    
    switch (type) {
        case LayoutType::KEEP_ORIGINAL: {
            // 保持原始位置，智能更新z-order和可见性
            for (size_t i = 0; i < _cards.size(); ++i) {
                // 保持原有的z-order，除非有冲突
                int currentZOrder = _cards[i]->getLocalZOrder();
                
                // 检查z-order冲突，如果有冲突则调整
                bool hasConflict = false;
                for (size_t j = 0; j < _cards.size(); ++j) {
                    if (i != j && _cards[j]->getLocalZOrder() == currentZOrder) {
                        hasConflict = true;
                        break;
                    }
                }
                
                // 如果有冲突或z-order不合理，重新分配
                if (hasConflict || currentZOrder < 0) {
                    _cards[i]->setLocalZOrder(static_cast<int>(i));
                    CCLOG("PlayfieldView: Adjusted z-order for card id=%d from %d to %d", 
                          _cards[i]->getCardId(), currentZOrder, static_cast<int>(i));
                }
                
                // 确保所有卡牌可见
                _cards[i]->setVisible(true);
                
                CCLOG("PlayfieldView: Keeping original position for card id=%d, zOrder=%d, pos=(%.1f, %.1f)", 
                      _cards[i]->getCardId(), _cards[i]->getLocalZOrder(), 
                      _cards[i]->getPosition().x, _cards[i]->getPosition().y);
            }
            break;
        }
        case LayoutType::HORIZONTAL_LINE: {
            // 水平排列
            const float cardSpacing = 120.0f;
            const float startX = 50.0f;
            const float y = 100.0f;
            
            for (size_t i = 0; i < _cards.size(); ++i) {
                float x = startX + static_cast<float>(i) * cardSpacing;
                _cards[i]->setPosition(Vec2(x, y));
                _cards[i]->setLocalZOrder(static_cast<int>(i));
                _cards[i]->setVisible(true);
                CCLOG("PlayfieldView: Horizontal layout card id=%d at pos=(%.1f, %.1f), zOrder=%d", 
                      _cards[i]->getCardId(), x, y, static_cast<int>(i));
            }
            break;
        }
        case LayoutType::CUSTOM: {
            // 自定义布局 - 确保所有卡牌可见且z-order正确
            for (size_t i = 0; i < _cards.size(); ++i) {
                _cards[i]->setVisible(true);
                if (_cards[i]->getLocalZOrder() < 0) {
                    _cards[i]->setLocalZOrder(static_cast<int>(i));
                }
            }
            break;
        }
    }
}

const std::vector<CardView*>& PlayfieldView::getCards() const {
    return _cards;
}

// 新增：保存卡片状态
void PlayfieldView::saveCardState(int cardId) {
    CardView* card = findCardById(cardId);
    if (card) {
        CardRestoreInfo info;
        info.cardId = cardId;
        info.position = card->getPosition();
        info.zOrder = card->getLocalZOrder();
        info.visible = card->isVisible();
        
        _cardStates[cardId] = info;
        
        CCLOG("PlayfieldView: Saved state for card id=%d, pos=(%.1f,%.1f), zOrder=%d, visible=%d", 
              cardId, info.position.x, info.position.y, info.zOrder, info.visible ? 1 : 0);
    }
}

// 新增：恢复卡片状态
void PlayfieldView::restoreCardState(int cardId) {
    auto it = _cardStates.find(cardId);
    if (it != _cardStates.end()) {
        CardView* card = findCardById(cardId);
        if (card) {
            const CardRestoreInfo& info = it->second;
            card->setPosition(info.position);
            card->setLocalZOrder(info.zOrder);
            card->setVisible(info.visible);
            
            CCLOG("PlayfieldView: Restored state for card id=%d, pos=(%.1f,%.1f), zOrder=%d, visible=%d", 
                  cardId, info.position.x, info.position.y, info.zOrder, info.visible ? 1 : 0);
        }
    }
}

// 新增：通过ID查找卡片
CardView* PlayfieldView::findCardById(int cardId) const {
    for (auto card : _cards) {
        if (card->getCardId() == cardId) {
            return card;
        }
    }
    return nullptr;
}
