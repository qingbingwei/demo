#pragma once
#include "models/GameModel.h"
#include "configs/models/LevelConfig.h"
class GameModelFromLevelGenerator {
public:
    static GameModel generateGameModel(const LevelConfig& config);
};