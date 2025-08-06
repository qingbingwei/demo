#pragma once
#include "cocos2d.h"
class AnimationUtils {
public:
    static void moveCardTo(cocos2d::Node* card, const cocos2d::Vec2& target, std::function<void()> onComplete);
};