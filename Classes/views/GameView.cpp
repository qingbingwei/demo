#include "GameView.h"
#include "PlayfieldView.h"
#include "StackView.h"
#include "CardView.h"
#include "configs/loaders/LevelConfigLoader.h"
#include "2d/CCLayer.h"
#include "2d/CCActionInterval.h"
#include "2d/CCActionInstant.h"
#include "base/CCDirector.h"
#include "ui/CocosGUI.h"
#include "controllers/GameController.h"

USING_NS_CC;

namespace {
    int nextCardId = 0;
}

bool GameView::init() {
    if (!Scene::init()) return false;

    this->setScale(1.0f);
    this->setRotation(0.0f);
    CCLOG("GameView scale=%f, rotation=%f", this->getScale(), this->getRotation());

    // 设置背景色
    auto bg = LayerColor::create(Color4B(34, 139, 34, 255));
    this->addChild(bg, -1);

    // 主牌区
    _playfieldView = PlayfieldView::create();
    if (!_playfieldView) {
        CCLOG("PlayfieldView::create() failed!");
        return false;
    }
    _playfieldView->setPosition(Vec2(0, 580));
    this->addChild(_playfieldView, 1);

    // 底牌堆（手牌区）
    _baseStackView = StackView::create();
    if (!_baseStackView) {
        CCLOG("BaseStackView::create() failed!");
        return false;
    }
    _baseStackView->setPosition(Vec2(450, 200));
    this->addChild(_baseStackView, 5);

    // 备用牌堆
    _reserveStackView = StackView::create();
    if (!_reserveStackView) {
        CCLOG("ReserveStackView::create() failed!");
        return false;
    }
    _reserveStackView->setPosition(Vec2(150, 200));
    this->addChild(_reserveStackView, 10);

    // 调试位置
    CCLOG("reserveStackView world pos: (%f, %f)", 
          _reserveStackView->convertToWorldSpace(Vec2(0, 0)).x, 
          _reserveStackView->convertToWorldSpace(Vec2(0, 0)).y);
    CCLOG("baseStackView world pos: (%f, %f)", 
          _baseStackView->convertToWorldSpace(Vec2(0, 0)).x, 
          _baseStackView->convertToWorldSpace(Vec2(0, 0)).y);

    // 创建控制器
    _controller = new GameController(this);
    if (!_controller) {
        CCLOG("GameView: Failed to create GameController");
        return false;
    }

    // 设置点击回调
    setOnCardClickCallback([this](int cardId) {
       CCLOG("GameView: Card click received, id=%d", cardId);
       if (_controller) {
           _controller->onCardClicked(cardId);
       }
    });

    // 撤销按钮
    _undoButton = ui::Button::create("button_undo_pressed.png");
    if (_undoButton) {
        CCLOG("Undo button created successfully, size: %f x %f", 
          _undoButton->getContentSize().width, _undoButton->getContentSize().height);
        _undoButton->addClickEventListener([this](Ref*) {
            if (_onUndoClickCallback) _onUndoClickCallback();
        });
        _undoButton->setPosition(Vec2(900, 200));
        this->addChild(_undoButton, 20);
        _undoButton->setVisible(true);
    } else {
        CCLOG("Failed to create undo button, check resources: button_undo_normal.png, button_undo_pressed.png");
    }

    // 加载关卡配置
    LevelConfig level = LevelConfigLoader::loadFromFile("level1.json");
    _controller->startGame(level);

    // 主牌区卡牌（从配置文件读取）
    // 关键：正序遍历，使用递增的z-order，确保先加载的卡牌在底层
    for (size_t i = 0; i < level.playfieldCards.size(); ++i) {
        const auto& cardCfg = level.playfieldCards[i];
        auto card = CardView::create(cardCfg.face, cardCfg.suit, true);
        if (card) {
            card->setCardId(nextCardId++);
            card->setPosition(cardCfg.position);
            
            // 设置z-order，先加载的卡牌设置较低的z-order，确保在下层
            // 直接使用i作为z-order，这样第一张卡z-order=0，最后一张最大
            int zOrder = static_cast<int>(i);
            card->setLocalZOrder(zOrder);
            
            card->setOnClickCallback([this](int cardId) {
                CCLOG("GameView: Forwarding playfield card click, id=%d", cardId);
                if (_onCardClickCallback) {
                    _onCardClickCallback(cardId);
                }
            });
            _playfieldView->addCard(card);
            CCLOG("Playfield card: id=%d, face=%d, suit=%d, pos=(%.1f,%.1f), zOrder=%d", 
                  card->getCardId(), cardCfg.face, cardCfg.suit, 
                  cardCfg.position.x, cardCfg.position.y, zOrder);
        }
    }

    // 备用牌堆卡牌（从配置文件读取）
    for (const auto& cardCfg : level.stackCards) {
        auto card = CardView::create(cardCfg.face, cardCfg.suit, true);
        if (card) {
            card->setCardId(nextCardId++);
            card->setPosition(cardCfg.position);
            card->setOnClickCallback([this](int cardId) {
                CCLOG("GameView: Reserve stack card clicked, id=%d", cardId);
                if (_onCardClickCallback) {
                    _onCardClickCallback(cardId);
                }
            });
            _reserveStackView->addCard(card);
            CCLOG("Added card to reserve stack: id=%d, face=%d, suit=%d", 
                  card->getCardId(), cardCfg.face, cardCfg.suit);
        }
    }

    // 手牌区卡牌（从配置文件读取，不再硬编码）
    for (const auto& cardCfg : level.baseCards) {
        auto card = CardView::create(cardCfg.face, cardCfg.suit, true);
        if (card) {
            card->setCardId(nextCardId++);
            card->setOnClickCallback([this](int cardId) {
                CCLOG("GameView: Base stack card clicked, id=%d", cardId);
                if (_onCardClickCallback) {
                    _onCardClickCallback(cardId);
                }
            });
            _baseStackView->addCard(card);
            CCLOG("Added card to base stack from config: id=%d, face=%d, suit=%d", 
                  card->getCardId(), cardCfg.face, cardCfg.suit);
        }
    }

    CCLOG("GameView initialization completed successfully!");
    CCLOG("Loaded %zu playfield cards, %zu reserve cards, %zu base cards", 
          level.playfieldCards.size(), level.stackCards.size(), level.baseCards.size());
    showUndoButton(true);
    return true;
}

void GameView::setOnCardClickCallback(const std::function<void(int)>& callback) {
    _onCardClickCallback = callback;
    if (_playfieldView) {
        _playfieldView->setOnCardClickCallback([this](int cardId) {
            CCLOG("GameView: Forwarding playfield card click, id=%d", cardId);
            if (_onCardClickCallback) {
                _onCardClickCallback(cardId);
            }
        });
    }
    if (_baseStackView) {
        _baseStackView->setOnCardClickCallback([this](int cardId) {
            CCLOG("GameView: Forwarding base stack card click, id=%d", cardId);
            if (_onCardClickCallback) {
                _onCardClickCallback(cardId);
            }
        });
    }
    if (_reserveStackView) {
        _reserveStackView->setOnCardClickCallback([this](int cardId) {
            CCLOG("GameView: Forwarding reserve stack card click, id=%d", cardId);
            if (_onCardClickCallback) {
                _onCardClickCallback(cardId);
            }
        });
    }
}

void GameView::setOnUndoClickCallback(const std::function<void()>& callback) {
    _onUndoClickCallback = callback;
    CCLOG("Undo callback set");
}

void GameView::showUndoButton(bool show) {
    if (_undoButton) {
        _undoButton->setVisible(show);
        CCLOG("Undo button visibility: %s", show ? "visible" : "hidden");
    }
}

void GameView::addCardToPlayfield(CardView* cardView) {
    if (!cardView || !_playfieldView) return;
    _playfieldView->addCard(cardView);
    CCLOG("GameView: Added playfield card, id=%d, pos=(%f, %f)", cardView->getCardId(), cardView->getPosition().x, cardView->getPosition().y);
}

void GameView::addCardToStack(CardView* cardView) {
    if (!cardView || !_baseStackView) return;
    
    auto topCard = _baseStackView->getTopCard();
    Vec2 targetPos = topCard ? topCard->getPosition() : Vec2(0, 0);
    
    cardView->retain();
    auto moveAction = MoveTo::create(0.3f, targetPos);
    auto callback = CallFunc::create([this, cardView, topCard]() {
        if (_baseStackView) {
            // 如果有顶部卡片并且是要实现覆盖效果，那么设置相同位置和更高的z-order
            if (topCard) {
                // 设置相同位置但更高的z-order
                cardView->setPosition(topCard->getPosition());
                cardView->setLocalZOrder(topCard->getLocalZOrder() + 1);
                cardView->setVisible(true); // 确保卡牌可见
                cardView->setOpacity(255);  // 确保完全不透明
                CCLOG("GameView: Card overlaid on top card with higher z-order: %d -> %d", 
                      topCard->getLocalZOrder(), cardView->getLocalZOrder());
            }
            
            _baseStackView->addCard(cardView);
            
            // 如果是覆盖效果，不要重新布局
            if (!topCard) {
                _baseStackView->layoutCards();
            }
        }
        cardView->release();
    });
    cardView->runAction(Sequence::create(moveAction, callback, nullptr));
    CCLOG("GameView: Added base stack card, id=%d, target pos=(%f, %f)", cardView->getCardId(), targetPos.x, targetPos.y);
}

void GameView::onHandCardClicked(CardView* cardView) {
    if (!cardView || !_baseStackView) return;
    auto topCard = _baseStackView->getTopCard();
    if (!topCard || cardView == topCard) {
        CCLOG("GameView: Card is already top card or no top card exists");
        return;
    }
    
    CCLOG("GameView: Moving hand card id=%d to top", cardView->getCardId());
    
    // 直接移动到顶部，不需要动画，因为布局会自动调整位置
    cardView->retain();
    auto callback = CallFunc::create([this, cardView]() {
        if (_baseStackView) {
            _baseStackView->moveCardToTop(cardView);
        }
        cardView->release();
    });
    // 使用短暂延迟确保操作顺序
    auto delay = DelayTime::create(0.01f);
    cardView->runAction(Sequence::create(delay, callback, nullptr));
}

void GameView::onReserveCardClicked(CardView* cardView) {
    if (!cardView || !_baseStackView || !_reserveStackView) return;
    
    // 计算目标位置：手牌区的新位置
    auto topCard = _baseStackView->getTopCard();
    Vec2 targetPos;
    
    if (topCard) {
        // 如果已经有顶部卡牌，新卡牌应该覆盖在它上面
        targetPos = topCard->getPosition();
        CCLOG("GameView: Reserve card will overlay top card at pos=(%.1f, %.1f)", targetPos.x, targetPos.y);
    } else {
        // 否则使用默认位置
        size_t baseCardCount = _baseStackView->getCards().size();
        targetPos = Vec2(static_cast<float>(baseCardCount) * 25.0f, 0.0f);
        CCLOG("GameView: Reserve card will be placed at default pos=(%.1f, %.1f)", targetPos.x, targetPos.y);
    }
    
    cardView->retain();
    auto moveAction = MoveTo::create(0.3f, targetPos);
    auto callback = CallFunc::create([this, cardView, topCard]() {
        if (_reserveStackView && _baseStackView) {
            _reserveStackView->removeCard(cardView);
            
            // 如果有顶部卡片并且是要实现覆盖效果，那么设置相同位置但更高的z-order
            if (topCard) {
                cardView->setPosition(topCard->getPosition());
                cardView->setLocalZOrder(topCard->getLocalZOrder() + 1);
                cardView->setVisible(true); // 确保卡牌可见
                cardView->setOpacity(255);  // 确保完全不透明
                CCLOG("GameView: Card overlaid on top card with higher z-order: %d -> %d", 
                     topCard->getLocalZOrder(), cardView->getLocalZOrder());
            }
            
            _baseStackView->addCard(cardView);
            _reserveStackView->layoutCards();
            
            // 如果是覆盖效果，不要对手牌区重新布局
            if (!topCard) {
                _baseStackView->layoutCards();
            }
        }
        cardView->release();
    });
    cardView->runAction(Sequence::create(moveAction, callback, nullptr));
    
    CCLOG("GameView: Moving reserve card id=%d to base stack, target pos=(%.1f, %.1f)", 
          cardView->getCardId(), targetPos.x, targetPos.y);
}