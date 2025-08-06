#pragma once
#include "cocos2d.h"
#include <vector>

struct CardConfig {
    int face;
    int suit;
    cocos2d::Vec2 position;
};

struct LevelConfig {
    std::vector<CardConfig> playfieldCards;   // 桌面牌区卡牌
    std::vector<CardConfig> stackCards;       // 备用牌堆卡牌
    std::vector<CardConfig> baseCards;        // 手牌区卡牌
};