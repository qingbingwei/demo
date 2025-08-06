#include "CardView.h"
#include "cocos2d.h"
#include "configs/loaders/LevelConfigLoader.h"
#include "controllers/GameController.h"
#include "StackView.h"
#include "PlayfieldView.h"

USING_NS_CC;

CardView* CardView::create(int face, int suit, bool faceUp) {
    CardView* card = new (std::nothrow) CardView();
    if (card && card->init(face, suit, faceUp)) {
        card->autorelease();
        return card;
    }
    CC_SAFE_DELETE(card);
    return nullptr;
}

bool CardView::init(int cardFace, int cardSuit, bool isFaceUp) {
    if (!Node::init()) return false;
    _cardFace = cardFace;
    _cardSuit = cardSuit;
    _isFaceUp = isFaceUp;
    _cardId = -1;

    // 卡牌底图
    _bgSprite = Sprite::create("card_general.png");
    if (!_bgSprite) {
        CCLOG("card_general.png NO!");
        return false;
    } else {
        CCLOG("card_general.png YES!");
    }
    this->addChild(_bgSprite);

    // 花色
    std::string suitPath;
    switch (_cardSuit) {
        case 0: suitPath = "club.png"; break;
        case 1: suitPath = "diamond.png"; break;
        case 2: suitPath = "heart.png"; break;
        case 3: suitPath = "spade.png"; break;
        default: suitPath = "club.png"; break;
    }
    _suitSprite = Sprite::create(suitPath);
    if (!_suitSprite) {
        CCLOG("%s NO!", suitPath.c_str());
        return false;
    } else {
        CCLOG("%s YES!", suitPath.c_str());
    }
    _suitSprite->setPosition(Vec2(33, 57));
    _bgSprite->addChild(_suitSprite);

    // 点数
    std::string numberPath = getNumberImagePath(cardFace, cardSuit);
    _numberSprite = Sprite::create(numberPath);
    if (!_numberSprite) {
        CCLOG("%s NO!", numberPath.c_str());
        return false;
    } else {
        CCLOG("%s YES!", numberPath.c_str());
    }
    _numberSprite->setPosition(Vec2(22, 26));
    _bgSprite->addChild(_numberSprite);

    // 缩放适配
    float targetCardWidth = 150;
    float scale = targetCardWidth / _bgSprite->getContentSize().width;
    _bgSprite->setScale(scale);
    CCLOG("Card position: (%f, %f)", this->getPosition().x, this->getPosition().y);

    // 强化点击事件处理 - 多重检查确保被覆盖的卡牌不能点击
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    
    // onTouchBegan - 首次触摸检测
    listener->onTouchBegan = [this](Touch* touch, Event* event) {
        // 基础可见性检查
        if (!this->isVisible() || this->getOpacity() == 0) {
            CCLOG("CardView: Card id=%d is not visible or transparent, touch ignored", _cardId);
            return false;
        }
        
        // 碰撞检测 - 确认触摸点在卡牌区域内
        Vec2 worldPos = touch->getLocation();
        Vec2 locationInNode = this->convertToNodeSpace(worldPos);
        Size s = _bgSprite->getContentSize() * _bgSprite->getScale();
        Rect rect(-s.width / 2, -s.height / 2, s.width, s.height);
        
        if (!rect.containsPoint(locationInNode)) {
            return false; // 触摸点不在卡牌区域内
        }
        
        // 覆盖检测 - 确认卡牌没有被其他卡牌覆盖
        auto parent = this->getParent();
        if (parent) {
            // 检查父节点是否是PlayfieldView
            auto playfieldView = dynamic_cast<PlayfieldView*>(parent);
            if (playfieldView) {
                // 执行覆盖检测
                if (playfieldView->isCardCovered(this)) {
                    CCLOG("CardView: Card id=%d is covered by other cards, touch ignored", _cardId);
                    return false;
                }
                
                // 额外的安全检查 - 验证此卡牌在可点击列表中
                bool isClickable = false;
                for (auto card : playfieldView->getCards()) {
                    // 只有未被覆盖的卡牌才能被点击
                    if (card == this && !playfieldView->isCardCovered(card)) {
                        isClickable = true;
                        break;
                    }
                }
                
                if (!isClickable) {
                    CCLOG("CardView: Card id=%d failed secondary clickability check, touch ignored", _cardId);
                    return false;
                }
            }
        }
        
        CCLOG("CardView: Touch check passed for card id=%d at position (%.1f,%.1f)", 
              _cardId, this->getPosition().x, this->getPosition().y);
        
        // 触发卡牌点击回调
        onCardClicked();
        return true;
    };
    
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    setFaceUp(isFaceUp);
    CCLOG("Card id=%d initialized, parent=%s", _cardId, 
          this->getParent() ? this->getParent()->getName().c_str() : "None");
    return true;
}

void CardView::setCardFace(int cardFace) { _cardFace = cardFace; }
void CardView::setCardSuit(int cardSuit) { _cardSuit = cardSuit; }
void CardView::setCardId(int cardId) { _cardId = cardId; }
int CardView::getCardId() const { return _cardId; }
void CardView::setOnClickCallback(const std::function<void(int)>& callback) { _onClickCallback = callback; }
void CardView::setFaceUp(bool isFaceUp) {
    _isFaceUp = isFaceUp;
    if (_suitSprite) {
        _suitSprite->setVisible(isFaceUp);
    }
    if (_numberSprite) {
        _numberSprite->setVisible(isFaceUp);
    }
    CCLOG("Card id=%d setFaceUp=%d", _cardId, isFaceUp);
}
void CardView::onCardClicked() { 
    if (_onClickCallback) _onClickCallback(_cardId); 
    CCLOG("Calling _onClickCallback for cardId=%d", _cardId);
}

std::string CardView::getNumberImagePath(int cardFace, int cardSuit) {
    std::string color = (cardSuit == 1 || cardSuit == 2) ? "red" : "black";
    std::string faceStr;
    switch (cardFace) {
        case 1: faceStr = "A"; break;
        case 2: faceStr = "2"; break;
        case 3: faceStr = "3"; break;
        case 4: faceStr = "4"; break;
        case 5: faceStr = "5"; break;
        case 6: faceStr = "6"; break;
        case 7: faceStr = "7"; break;
        case 8: faceStr = "8"; break;
        case 9: faceStr = "9"; break;
        case 10: faceStr = "10"; break;
        case 11: faceStr = "J"; break;
        case 12: faceStr = "Q"; break;
        case 13: faceStr = "K"; break;
        default: faceStr = "A"; break;
    }
    return "big_" + color + "_" + faceStr + ".png";
}