#ifndef __GAME_SCENE_H__
#define __GAME_SCENE_H__

#include "cocos2d.h"
#include "views/GameView.h"

class GameScene : public cocos2d::Scene
{
public:
    static GameScene* create();
    virtual bool init();
};

#endif // __GAME_SCENE_H__
