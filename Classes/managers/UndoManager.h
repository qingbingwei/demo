#ifndef UNDO_MANAGER_H
#define UNDO_MANAGER_H

#include <stack>
#include "cocos2d.h"

enum class MoveType {
    RESERVE_TO_BASE,
    REORDER_BASE,
    PLAYFIELD_TO_BASE
};

struct UndoRecord {
    int cardId;
    MoveType moveType;
    cocos2d::Vec2 originalPos;
    int originalParent; // 0: Playfield, 1: Reserve, 2: Base
    int originalIndex = -1; // 用于记录在原始容器中的位置索引，便于精确恢复
};

class UndoManager {
public:
    void recordMove(const UndoRecord& record);
    bool canUndo() const;
    UndoRecord undo();
    void push(const UndoRecord& record);

private:
    std::stack<UndoRecord> undoStack;
};

#endif // UNDO_MANAGER_H