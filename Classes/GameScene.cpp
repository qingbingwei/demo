#include "GameScene.h"
USING_NS_CC;

GameScene* GameScene::create() {
    GameScene* ret = new (std::nothrow) GameScene();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

/**
 * @brief 初始化游戏场景。
 * @return 初始化成功返回 true，否则返回 false。
 */
bool GameScene::init() {
    if (!Scene::init()) return false;
    auto gameView = GameView::create();
    //CCLOG("Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!");
    if (!gameView) {
        CCLOG("GameView create failed!");
        return false;
    } else {
        CCLOG("GameView create success!");
    }
    this->addChild(gameView);
    return true;
}