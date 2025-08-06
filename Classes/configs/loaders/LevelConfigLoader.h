// configs/loaders/LevelConfigLoader.h
#pragma once
#include "configs/models/LevelConfig.h"
#include <string>

class LevelConfigLoader {
public:
    // 加载指定关卡配置
    static LevelConfig loadFromFile(const std::string& filename);
};