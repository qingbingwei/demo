
/**
 * @file GameController.cpp
 * @brief 游戏控制器实现，负责主流程、交互逻辑和视图更新。
 *
 * 设计说明：
 * 1. 连接 GameModel 与各视图，处理用户操作和游戏规则。
 * 2. 主要职责包括：卡牌点击、撤销、匹配、视图同步。
 * 3. 所有私有成员均以 _ 下划线开头，命名风格规范。
 */
#include "controllers/GameController.h"
#include "cocos2d.h"
#include "managers/UndoManager.h"
#include <algorithm>

USING_NS_CC;

/**
 * @brief 构造函数，初始化控制器并绑定视图。
 * @param view 游戏主视图。
 */
GameController::GameController(GameView* view) : _gameView(view) {
    CCLOG("GameController initialized, setting callback for view=%p", _gameView);
    if (_gameView) {
        _gameView->setOnCardClickCallback([this](int cardId) {
            CCLOG("GameController: Card clicked, id=%d", cardId);
            onCardClicked(cardId);
        });
        _gameView->setOnUndoClickCallback([this]() {
            CCLOG("GameController: Undo clicked");
            onUndoClicked();
        });
    }
}


/**
 * @brief 启动游戏，加载关卡配置。
 * @param config 关卡配置。
 */
void GameController::startGame(const LevelConfig& config) {
    CCLOG("Starting game with %zu stack cards, %zu playfield cards, %zu base cards",
          config.stackCards.size(), config.playfieldCards.size(), config.baseCards.size());

    _gameModel.clear();

    // 加载备用牌堆卡牌
    for (const auto& cardCfg : config.stackCards) {
        CardModel card;
        card.id = _gameModel.getNextCardId();
        card.face = cardCfg.face;
        card.suit = cardCfg.suit;
        card.isFaceUp = true;
        card.isRemoved = false;
        card.posX = cardCfg.position.x;
        card.posY = cardCfg.position.y;
        _gameModel.addCardToReserveStack(card);
    }

    // 加载桌面牌区卡牌
    for (const auto& cardCfg : config.playfieldCards) {
        CardModel card;
        card.id = _gameModel.getNextCardId();
        card.face = cardCfg.face;
        card.suit = cardCfg.suit;
        card.isFaceUp = true;
        card.isRemoved = false;
        card.posX = cardCfg.position.x;
        card.posY = cardCfg.position.y;
        _gameModel.addCardToPlayfield(card);
    }

    // 加载手牌区卡牌
    for (const auto& cardCfg : config.baseCards) {
        CardModel card;
        card.id = _gameModel.getNextCardId();
        card.face = cardCfg.face;
        card.suit = cardCfg.suit;
        card.isFaceUp = true;
        card.isRemoved = false;
        card.posX = cardCfg.position.x;
        card.posY = cardCfg.position.y;
        _gameModel.addCardToBaseStack(card);
    }

    updateView();
}

// 改进的匹配函数，支持A-K循环匹配

/**
 * @brief 判断两张卡牌是否可匹配（支持A-K循环）。
 * @param face1 第一张卡牌面值。
 * @param face2 第二张卡牌面值。
 * @return 是否可匹配。
 */
bool GameController::canMatchCards(int face1, int face2) {
    // 标准匹配：点数差1
    if (std::abs(face1 - face2) == 1) {
        return true;
    }

    // A-K循环匹配：A(1)可以匹配K(13)，K(13)可以匹配A(1)
    if ((face1 == 1 && face2 == 13) || (face1 == 13 && face2 == 1)) {
        return true;
    }

    return false;
}

void GameController::onCardClicked(int cardId) {
    auto playfieldView = _gameView->getPlayfieldView();
    auto baseStackView = _gameView->getBaseStackView();
    auto reserveStackView = _gameView->getReserveStackView();
    CCLOG("GameController: onCardClicked called for cardId=%d", cardId);
    
    // 查找卡牌来源
    CardView* cardView = nullptr;
    bool fromPlayfield = false, fromReserve = false, fromBase = false;

    // 检查主牌区
    cardView = findCardViewById(cardId, playfieldView);
    if (cardView) {
        fromPlayfield = true;
        // 1. 第一层覆盖检查
        if (playfieldView->isCardCovered(cardView)) {
            CCLOG("GameController: Playfield card id=%d is covered, ignoring click", cardId);
            return;
        }
        
        // 2. 额外检查：确保卡牌在可点击列表中
        auto clickableCards = getClickablePlayfieldCards();
        bool isInClickableList = false;
        
        for (auto card : clickableCards) {
            if (card->getCardId() == cardId) {
                isInClickableList = true;
                break;
            }
        }
        
        if (!isInClickableList) {
            CCLOG("GameController: Playfield card id=%d is not in clickable cards list, ignoring click", cardId);
            return;
        }
    } else {
        // 检查是否在其他区域
        cardView = findCardViewById(cardId, reserveStackView);
        if (cardView) {
            fromReserve = true;
        } else {
            cardView = findCardViewById(cardId, baseStackView);
            if (cardView) {
                fromBase = true;
            }
        }
    }

    if (!cardView) {
        CCLOG("GameController: Card id=%d not found", cardId);
        return;
    }
    
    // 3. 第三层覆盖检查（针对桌面牌）
    if (fromPlayfield) {
        // 再次确认没有被覆盖
        if (playfieldView->isCardCovered(cardView)) {
            CCLOG("GameController: Playfield card id=%d is covered (triple-check), ignoring click", cardId);
            return;
        }
        
        // 手动覆盖检测 - 确保Z轴顺序更高的卡牌没有覆盖此卡牌
        int targetZOrder = cardView->getLocalZOrder();
        Vec2 targetPos = cardView->getPosition();
        Size targetSize = cardView->getContentSize();
        
        if (targetSize.width <= 0 || targetSize.height <= 0) {
            targetSize = Size(150.0f, 210.0f);
        }
        
        for (auto otherCard : playfieldView->getCards()) {
            if (otherCard == cardView || !otherCard->isVisible() || otherCard->getOpacity() == 0)
                continue;
                
            if (otherCard->getLocalZOrder() > targetZOrder) {
                Vec2 otherPos = otherCard->getPosition();
                Size otherSize = otherCard->getContentSize();
                
                if (otherSize.width <= 0 || otherSize.height <= 0) {
                    otherSize = Size(150.0f, 210.0f);
                }
                
                // 检查是否有重叠
                float left1 = targetPos.x - targetSize.width/2;
                float right1 = targetPos.x + targetSize.width/2;
                float bottom1 = targetPos.y - targetSize.height/2;
                float top1 = targetPos.y + targetSize.height/2;
                
                float left2 = otherPos.x - otherSize.width/2;
                float right2 = otherPos.x + otherSize.width/2;
                float bottom2 = otherPos.y - otherSize.height/2;
                float top2 = otherPos.y + otherSize.height/2;
                
                if (left1 < right2 && right1 > left2 && bottom1 < top2 && top1 > bottom2) {
                    CCLOG("GameController: Manual coverage check - card id=%d is covered by card id=%d, ignoring click", 
                          cardId, otherCard->getCardId());
                    return;
                }
            }
        }
    }
    
    CCLOG("GameController: Card clicked, fromReserve=%d, fromBase=%d, fromPlayfield=%d", 
          fromReserve, fromBase, fromPlayfield);

    // 1. 处理手牌区非顶部牌的点击 (需求1: 手牌区翻牌替换)
    if (fromBase && cardView != baseStackView->getTopCard()) {
        // 检查是否有可匹配的桌面牌 - 但不阻止用户操作
        bool canMatchAny = false;
        auto topCard = baseStackView->getTopCard();
        
        if (topCard) {
            // 创建一个函数来获取所有可匹配的卡牌 - 抽取成独立逻辑便于复用和调试
            auto matchableCards = getMatchablePlayfieldCards(topCard);
            
            canMatchAny = !matchableCards.empty();
            
            if (canMatchAny) {
                CCLOG("GameController: Found %zu matchable cards on playfield", matchableCards.size());
                for (auto& card : matchableCards) {
                    CCLOG("GameController: Matchable card: id=%d, face=%d, pos=(%.1f,%.1f)", 
                          card->getCardId(), card->getCardFace(),
                          card->getPosition().x, card->getPosition().y);
                }
            }
        }
        
        // 即使有可匹配的桌面牌，也允许翻手牌
        // 保存撤销状态
        UndoRecord record;
        record.cardId = cardId;
        record.moveType = MoveType::REORDER_BASE;
        record.originalPos = cardView->getPosition();
        record.originalParent = 2; // 手牌区
        
        // 记录重排序前的状态 - 记录所有卡牌的当前顺序
        const auto& cards = baseStackView->getCards();
        for (size_t i = 0; i < cards.size(); ++i) {
            if (cards[i] == cardView) {
                record.originalIndex = static_cast<int>(i);
                break;
            }
        }
        
        // 创建手牌区状态快照用于撤销
        _baseStackSnapshot.clear();
        for (auto card : cards) {
            _baseStackSnapshot.push_back(card);
        }
        
        // 执行手牌翻转操作
        _gameView->onHandCardClicked(cardView);
        _moveHistory.push({cardId, false, true});
        _undoManager.push(record);
        _gameView->showUndoButton(true);
        
        if (canMatchAny) {
            CCLOG("GameController: Hand card flipped even though matches exist on playfield");
        } else {
            CCLOG("GameController: Hand card flipped, no matches found on playfield");
        }
        
        CCLOG("GameController: Moved card id=%d to base stack top, original index was %d", 
              cardId, record.originalIndex);
    }
    
    // 2. 处理桌面牌与手牌区顶部牌的匹配 (需求2: 桌面牌与手牌区顶部牌匹配)
    else if (fromPlayfield) {
        // 再次确认桌面牌没有被覆盖
        if (playfieldView->isCardCovered(cardView)) {
            CCLOG("GameController: Playfield card id=%d is covered (quadruple-check), ignoring click", cardId);
            return;
        }
        
        auto topCard = baseStackView->getTopCard();
        if (topCard) {
            // 使用改进的匹配逻辑，支持A-K循环匹配
            int playfieldCardFace = cardView->getCardFace();
            int baseTopCardFace = topCard->getCardFace();
            
            CCLOG("GameController: Checking match - playfield card face=%d, base top card face=%d", 
                  playfieldCardFace, baseTopCardFace);
            
            if (canMatchCards(playfieldCardFace, baseTopCardFace)) {
                UndoRecord record;
                record.cardId = cardId;
                record.moveType = MoveType::PLAYFIELD_TO_BASE;
                record.originalPos = cardView->getPosition();
                record.originalParent = 0; // 桌面牌区
                
                CCLOG("GameController: Cards match! Moving playfield card id=%d to base stack", cardId);
                
                // 移动前保存卡片在桌面区的状态，用于撤销操作
                _gameView->getPlayfieldView()->saveCardState(cardId);
                
                // 保留对桌面牌的引用，但不从桌面区移除它
                cardView->retain(); // 防止被释放

                // 创建一个新卡片作为覆盖层，保留原始卡片在桌面上
                auto overlayCard = CardView::create(cardView->getCardFace(), cardView->getCardSuit(), true);
                overlayCard->setCardId(cardId); // 使用相同的ID以保持一致性

                // 设置目标位置为手牌区顶部卡片
                Vec2 targetPos = topCard->getPosition();
                overlayCard->setPosition(cardView->getPosition()); // 从原始位置开始
                overlayCard->setVisible(true);
                overlayCard->setOpacity(255);

                // 设置点击回调
                overlayCard->setOnClickCallback([this](int cardId) {
                    if (_gameView && _gameView->getBaseStackView()) {
                        auto baseStackView = _gameView->getBaseStackView();
                        auto cardView = baseStackView->findCardById(cardId);
                        if (cardView && cardView == baseStackView->getTopCard()) {
                            CCLOG("GameController: Top overlay card clicked, id=%d", cardId);
                            // 顶部卡点击无特殊操作
                        }
                    }
                });

                // 将覆盖卡添加到手牌区的父节点（场景）以便进行移动动画
                _gameView->addChild(overlayCard, 999);
                
                // 创建移动动画
                auto moveAction = MoveTo::create(0.3f, targetPos);
                auto callback = CallFunc::create([this, overlayCard, topCard, cardView]() {
                    if (_gameView && _gameView->getBaseStackView()) {
                        // 从场景中移除覆盖卡
                        _gameView->removeChild(overlayCard);
                        
                        // 将卡牌放在手牌区顶部卡片的上方(覆盖)
                        overlayCard->setPosition(topCard->getPosition());
                        overlayCard->setLocalZOrder(topCard->getLocalZOrder() + 1);
                        overlayCard->setVisible(true); // 确保卡牌可见
                        overlayCard->setOpacity(255);  // 确保完全不透明
                        
                        // 添加到手牌区
                        _gameView->getBaseStackView()->addCard(overlayCard);
                        
                        CCLOG("GameController: Card successfully overlaid on top card in base stack");
                        
                        // 使桌面上的原始卡片变为透明（但保留在原位置）
                        cardView->setOpacity(0);
                        cardView->setVisible(false);
                    }
                    cardView->release(); // 在回调完成后释放
                });
                
                overlayCard->runAction(Sequence::create(moveAction, callback, nullptr));
                _moveHistory.push({cardId, true, true});
                _undoManager.push(record);
                _gameView->showUndoButton(true);
                CCLOG("GameController: Moved card id=%d from playfield to overlay base stack top card", cardId);
            } else {
                CCLOG("GameController: Card id=%d cannot match top card, faces %d and %d don't match", 
                      cardId, playfieldCardFace, baseTopCardFace);
            }
        } else {
            CCLOG("GameController: No top card in base stack to match with");
        }
    }
    
    // 3. 处理备用牌堆的点击
    else if (fromReserve && cardView == reserveStackView->getTopCard()) {
        bool canMatchAny = false;
        auto topCard = baseStackView->getTopCard();
        
        if (topCard) {
            // 使用辅助方法检查是否有可匹配的卡牌
            auto matchableCards = getMatchablePlayfieldCards(topCard);
            canMatchAny = !matchableCards.empty();
            
            if (canMatchAny) {
                CCLOG("GameController: Found %zu matchable cards on playfield", matchableCards.size());
                for (auto& card : matchableCards) {
                    CCLOG("GameController: Matchable card: id=%d, face=%d, pos=(%.1f,%.1f)", 
                          card->getCardId(), card->getCardFace(),
                          card->getPosition().x, card->getPosition().y);
                }
            }
        }
        
        // 允许从备用牌堆抽牌，不受桌面可匹配牌的限制
        CCLOG("GameController: %s", canMatchAny ? 
              "Matches exist on playfield but allowing reserve card draw anyway" :
              "No matching cards found, allowing reserve card draw");
        
        UndoRecord record;
        record.cardId = cardId;
        record.moveType = MoveType::RESERVE_TO_BASE;
        record.originalPos = cardView->getPosition();
        record.originalParent = 1; // 备用牌堆
        
        // 保存备用牌堆状态以便撤销
        reserveStackView->saveCardState(cardId);
        
        _gameView->onReserveCardClicked(cardView);
        _moveHistory.push({cardId, false, true});
        _undoManager.push(record);
        CCLOG("GameController: Moved card id=%d from reserve to base stack", cardId);
        _gameView->showUndoButton(true);
    }
    
    // 4. 处理手牌区顶部牌的点击 (无操作)
    else if (fromBase && cardView == baseStackView->getTopCard()) {
        CCLOG("GameController: Base stack top card id=%d clicked, no action needed", cardId);
    }
}

void GameController::onUndoClicked() {
    if (!_undoManager.canUndo()) {
        CCLOG("GameController: No actions to undo");
        return;
    }

    // 只取出一条撤销记录进行处理
    UndoRecord record = _undoManager.undo();
    CCLOG("GameController: Undoing action - cardId=%d, type=%d", record.cardId, static_cast<int>(record.moveType));

    // 根据原始父容器查找卡牌
    CardView* cardView = nullptr;
    
    // 从当前位置查找卡牌
    if (record.originalParent == 0) { // 原来在桌面牌区
        cardView = findCardViewById(record.cardId, _gameView->getBaseStackView());
    } else if (record.originalParent == 1) { // 原来在备用牌堆
        cardView = findCardViewById(record.cardId, _gameView->getBaseStackView());
    } else if (record.originalParent == 2) { // 原来在手牌区（仅位置改变）
        cardView = findCardViewById(record.cardId, _gameView->getBaseStackView());
    }

    if (!cardView) {
        CCLOG("GameController: Card id=%d not found for undo", record.cardId);
        // 更新撤销按钮状态
        _gameView->showUndoButton(_undoManager.canUndo());
        return;
    }
    
    CCLOG("GameController: Found card id=%d, starting undo movement", record.cardId);
    
    cardView->retain(); // 防止被释放
    switch (record.moveType) {
        case MoveType::RESERVE_TO_BASE: {
            // 从手牌区移回备用牌堆
            auto moveAction = MoveTo::create(0.3f, record.originalPos);
            auto callback = CallFunc::create([this, cardView]() {
                if (_gameView) {
                    _gameView->getBaseStackView()->removeCard(cardView);
                    _gameView->getReserveStackView()->addCard(cardView);
                    _gameView->getBaseStackView()->layoutCards();
                    _gameView->getReserveStackView()->layoutCards();
                    CCLOG("GameController: Undo complete - card moved back to reserve stack");
                }
                cardView->release();
                
                // 更新撤销按钮状态
                _gameView->showUndoButton(_undoManager.canUndo());
                updateView();
            });
            cardView->runAction(Sequence::create(moveAction, callback, nullptr));
            break;
        }
        case MoveType::REORDER_BASE: {
            // 手牌区内部位置恢复 - 使用快照恢复顺序
            auto callback = CallFunc::create([this, cardView]() {
                if (_gameView && !_baseStackSnapshot.empty()) {
                    auto baseStackView = _gameView->getBaseStackView();
                    
                    // 恢复到快照状态
                    restoreBaseStackOrder(baseStackView, _baseStackSnapshot);
                    
                    CCLOG("GameController: Undo complete - base stack order restored from snapshot");
                }
                cardView->release();
                
                // 更新撤销按钮状态
                _gameView->showUndoButton(_undoManager.canUndo());
                updateView();
            });
            auto delay = DelayTime::create(0.1f);
            cardView->runAction(Sequence::create(delay, callback, nullptr));
            break;
        }
        case MoveType::PLAYFIELD_TO_BASE: {
            // 从手牌区移回桌面牌区 - 改进逻辑，确保原始卡片重新可见
            CCLOG("GameController: Starting PLAYFIELD_TO_BASE undo for card id=%d", record.cardId);
            
            // 1. 从手牌区找到并移除覆盖卡
            CardView* overlayCard = findCardViewById(record.cardId, _gameView->getBaseStackView());
            if (overlayCard) {
                overlayCard->stopAllActions();
                _gameView->getBaseStackView()->removeCard(overlayCard);
            }
            
            if (_gameView) {
                auto playfieldView = _gameView->getPlayfieldView();
                
                // 2. 找到桌面上的原始卡片并恢复可见性
                CardView* originalCard = nullptr;
                for (auto card : playfieldView->getCards()) {
                    if (card->getCardId() == record.cardId) {
                        originalCard = card;
                        break;
                    }
                }
                
                if (originalCard) {
                    // 3. 恢复原始卡片的可见性和位置
                    originalCard->setVisible(true);
                    originalCard->setOpacity(255); // 确保完全不透明
                    originalCard->setPosition(record.originalPos);
                    
                    // 4. 尝试恢复保存的状态
                    playfieldView->restoreCardState(record.cardId);
                    
                    CCLOG("GameController: Restored original card id=%d in playfield at pos=(%.1f, %.1f)", 
                          record.cardId, originalCard->getPosition().x, originalCard->getPosition().y);
                } else {
                    CCLOG("GameController: Original card not found in playfield, cannot restore");
                }
                
                // 5. 重新布局手牌区
                _gameView->getBaseStackView()->layoutCards();
            }
            
            cardView->release();
            
            // 更新撤销按钮状态
            _gameView->showUndoButton(_undoManager.canUndo());
            updateView();
            break;
        }
    }
    
    // 增强的最终验证：确保所有桌面牌可见且位置正确
    if (_gameView && _gameView->getPlayfieldView()) {
        auto playfieldView = _gameView->getPlayfieldView();
        
        // 延迟检查所有卡片的可见性和位置
        Director::getInstance()->getScheduler()->schedule(
            [this, playfieldView](float) {
                CCLOG("GameController: Final verification of all playfield cards");
                for (auto card : playfieldView->getCards()) {
                    // 忽略已经匹配的（不可见）的卡牌
                    if (card->getOpacity() == 0) continue;
                    
                    // 1. 检查可见性
                    if (!card->isVisible()) {
                        CCLOG("GameController: Final check - card id=%d is invisible, fixing", card->getCardId());
                        card->setVisible(true);
                        card->setOpacity(255);
                    }
                    
                    // 2. 确保z-order合理
                    if (card->getLocalZOrder() < 0) {
                        CCLOG("GameController: Final check - card id=%d has negative z-order, fixing", card->getCardId());
                        card->setLocalZOrder(10); // 使用较高的z-order
                    }
                }
            },
            this, 0.2f, 0, 0, false, "final_playfield_check"
        );
    }
    
    CCLOG("GameController: Undo operation complete, remaining undos available: %s", 
          _undoManager.canUndo() ? "yes" : "no");
}

// 新增：恢复手牌区顺序的辅助方法
void GameController::restoreBaseStackOrder(StackView* baseStackView, const std::vector<CardView*>& snapshot) {
    if (!baseStackView || snapshot.empty()) return;
    
    CCLOG("GameController: Restoring base stack order from snapshot with %zu cards", snapshot.size());
    
    // 清除当前的卡牌容器但不删除卡牌对象
    auto& currentCards = baseStackView->getCards();
    currentCards.clear();
    
    // 按快照顺序重新添加卡牌
    for (auto card : snapshot) {
        if (card) {
            currentCards.push_back(card);
        }
    }
    
    // 重新布局
    baseStackView->layoutCards();
    
    CCLOG("GameController: Base stack order restored, now has %zu cards", currentCards.size());
}

void GameController::updateView() {
    if (_gameView) {
        _gameView->showUndoButton(_undoManager.canUndo());
        
        // 检查并修复桌面区卡片可见性问题
        auto playfieldView = _gameView->getPlayfieldView();
        if (playfieldView) {
            // 确保所有桌面卡片有合理的z-order和可见性设置
            for (auto card : playfieldView->getCards()) {
                // 如果卡片被标记为匹配（透明度为0）则继续保持透明
                if (card->getOpacity() == 0) continue;
                
                // 否则确保卡片可见
                if (!card->isVisible()) {
                    card->setVisible(true);
                    CCLOG("GameController: Fixed visibility for playfield card id=%d", card->getCardId());
                }
                
                // 确保有合理的Z-order
                if (card->getLocalZOrder() < 0) {
                    card->setLocalZOrder(10); // 使用合理的默认值
                    CCLOG("GameController: Fixed z-order for playfield card id=%d", card->getCardId());
                }
            }
        }
    }
}

bool GameController::canMatch(int playfieldCardId, int stackTopCardId) {
    // 通过查找相应的视图对象来判断是否可以匹配
    auto playfieldView = _gameView->getPlayfieldView();
    auto baseStackView = _gameView->getBaseStackView();
    
    CardView* playfieldCard = findCardViewById(playfieldCardId, playfieldView);
    CardView* stackCard = findCardViewById(stackTopCardId, baseStackView);
    
    if (!playfieldCard || !stackCard) {
        return false;
    }
    
    // 使用改进的匹配逻辑，支持A-K循环匹配
    return canMatchCards(playfieldCard->getCardFace(), stackCard->getCardFace());
}

CardView* GameController::findCardViewById(int cardId, PlayfieldView* view) {
    for (CardView* card : view->getCards()) {
        if (card->getCardId() == cardId) {
            return card;
        }
    }
    return nullptr;
}

CardView* GameController::findCardViewById(int cardId, StackView* view) {
    for (CardView* card : view->getCards()) {
        if (card->getCardId() == cardId) {
            return card;
        }
    }
    return nullptr;
}

// 新增：获取桌面上可点击的卡牌（可见且未被覆盖）- 强化版
std::vector<CardView*> GameController::getClickablePlayfieldCards() {
    std::vector<CardView*> result;
    auto playfieldView = _gameView->getPlayfieldView();
    if (!playfieldView) {
        CCLOG("GameController: PlayfieldView is null in getClickablePlayfieldCards");
        return result;
    }
    
    CCLOG("GameController: Starting search for clickable cards");
    
    // 获取所有卡牌
    const auto& allCards = playfieldView->getCards();
    CCLOG("GameController: Found %zu total cards in playfield", allCards.size());
    
    // 按Z轴顺序排序卡牌（从底到顶）
    std::vector<CardView*> sortedCards = allCards;
    std::sort(sortedCards.begin(), sortedCards.end(), 
        [](CardView* a, CardView* b) {
            return a->getLocalZOrder() < b->getLocalZOrder();
        });
    
    // 找出所有可见且不透明的卡牌
    std::vector<CardView*> visibleCards;
    for (auto card : sortedCards) {
        if (card->isVisible() && card->getOpacity() > 0) {
            visibleCards.push_back(card);
        } else {
            CCLOG("GameController: Card id=%d is invisible or transparent, skipping", 
                  card->getCardId());
        }
    }
    
    CCLOG("GameController: Found %zu visible cards on playfield", visibleCards.size());
    
    // 进行多次覆盖检查
    for (auto card : visibleCards) {
        bool isCovered = false;
        
        // 检查1: 使用PlayfieldView的isCardCovered方法
        isCovered = playfieldView->isCardCovered(card);
        
        if (isCovered) {
            CCLOG("GameController: Card id=%d is covered according to PlayfieldView, skipping", 
                  card->getCardId());
            continue;
        }
        
        // 检查2: 手动进行Z轴顺序覆盖检测
        int cardZOrder = card->getLocalZOrder();
        for (auto otherCard : visibleCards) {
            if (otherCard == card) continue;
            
            // 只检查Z轴顺序更高的卡牌
            if (otherCard->getLocalZOrder() > cardZOrder) {
                // 计算两个卡牌的边界，检查是否有任何重叠
                Vec2 cardPos = card->getPosition();
                Vec2 otherPos = otherCard->getPosition();
                
                // 卡牌尺寸估算（使用实际尺寸或默认值）
//                 Size cardSize = card->getContentSize();
//                 Size otherSize = otherCard->getContentSize();
                
                Size cardSize = Size(150.0f, 210.0f);
                Size otherSize = Size(150.0f, 210.0f);
                
                // 使用更大的安全边距进行覆盖检测
                const float SAFETY_MARGIN = 10.0f;
                
                // 计算两个卡牌的边界
                float card1Left = cardPos.x - (cardSize.width/2) - SAFETY_MARGIN;
                float card1Right = cardPos.x + (cardSize.width/2) + SAFETY_MARGIN;
                float card1Bottom = cardPos.y - (cardSize.height/2) - SAFETY_MARGIN;
                float card1Top = cardPos.y + (cardSize.height/2) + SAFETY_MARGIN;
                
                float card2Left = otherPos.x - (otherSize.width/2);
                float card2Right = otherPos.x + (otherSize.width/2);
                float card2Bottom = otherPos.y - (otherSize.height/2);
                float card2Top = otherPos.y + (otherSize.height/2);
                
                // 检查是否有任何重叠
                if (card1Left < card2Right && card1Right > card2Left &&
                    card1Bottom < card2Top && card1Top > card2Bottom) {
                    CCLOG("GameController: Card id=%d is covered by card id=%d (manual check), skipping",
                         card->getCardId(), otherCard->getCardId());
                    isCovered = true;
                    break;
                }
            }
        }
        
        if (isCovered) {
            continue; // 跳过被覆盖的卡牌
        }
        
        // 如果通过了所有检查，则添加到结果中
        result.push_back(card);
        CCLOG("GameController: Card id=%d at pos=(%.1f,%.1f) is not covered and can be clicked", 
             card->getCardId(), card->getPosition().x, card->getPosition().y);
    }
    
    // 打印最终可点击卡牌列表
    CCLOG("GameController: Final list of %zu uncovered cards on playfield that can be clicked:", result.size());
    for (auto card : result) {
        CCLOG("  - Card id=%d, face=%d, suit=%d, pos=(%.1f,%.1f), zOrder=%d", 
              card->getCardId(), card->getCardFace(), card->getCardSuit(),
              card->getPosition().x, card->getPosition().y, card->getLocalZOrder());
    }
    
    return result;
}

// 查找与指定卡牌可匹配的所有桌面牌 - 强化版
std::vector<CardView*> GameController::getMatchablePlayfieldCards(CardView* cardToMatch) {
    std::vector<CardView*> result;
    if (!cardToMatch) {
        CCLOG("GameController: cardToMatch is null in getMatchablePlayfieldCards");
        return result;
    }
    
    if (!_gameView || !_gameView->getPlayfieldView()) {
        CCLOG("GameController: GameView or PlayfieldView is null in getMatchablePlayfieldCards");
        return result;
    }
    
    auto playfieldView = _gameView->getPlayfieldView();
    
    // 获取所有可点击的卡牌（这里已经确保了卡牌未被覆盖）
    auto clickableCards = getClickablePlayfieldCards();
    
    CCLOG("GameController: Checking for matches among %zu clickable cards for card face=%d", 
          clickableCards.size(), cardToMatch->getCardFace());
    
    // 检查每张可点击卡牌是否能与传入的卡牌匹配
    for (auto card : clickableCards) {
        // 三重检查: 再次确认卡牌未被覆盖
        if (playfieldView->isCardCovered(card)) {
            CCLOG("GameController: Card id=%d is covered (triple-check), excluding from matches", 
                  card->getCardId());
            continue;
        }
        
        // 检查卡牌匹配规则
        if (canMatchCards(card->getCardFace(), cardToMatch->getCardFace())) {
            CCLOG("GameController: Card id=%d (face=%d) matches with card face=%d", 
                 card->getCardId(), card->getCardFace(), cardToMatch->getCardFace());
            
            // 最终检查: 确认卡牌可见性和透明度
            if (card->isVisible() && card->getOpacity() > 0) {
                result.push_back(card);
                CCLOG("GameController: Added matching card to result: id=%d, face=%d, pos=(%.1f,%.1f)", 
                      card->getCardId(), card->getCardFace(), card->getPosition().x, card->getPosition().y);
            } else {
                CCLOG("GameController: Card id=%d matches but is not visible or transparent", card->getCardId());
            }
        }
    }
    
    // 打印匹配结果摘要
    if (result.empty()) {
        CCLOG("GameController: No uncovered cards match with card face=%d", cardToMatch->getCardFace());
    } else {
        CCLOG("GameController: Found %zu uncovered matching cards for card face=%d", 
              result.size(), cardToMatch->getCardFace());
        
        // 详细打印每张匹配卡牌
        for (auto card : result) {
            CCLOG("GameController: Matched card details: id=%d, face=%d, suit=%d, pos=(%.1f,%.1f), zOrder=%d", 
                  card->getCardId(), card->getCardFace(), card->getCardSuit(), 
                  card->getPosition().x, card->getPosition().y, card->getLocalZOrder());
        }
    }
    
    return result;
}