#pragma once
#include "GameModel.h"
struct UndoStep {
    GameModel snapshot;
};
struct UndoModel {
    std::vector<UndoStep> steps;
};