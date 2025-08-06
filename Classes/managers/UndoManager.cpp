#include "managers/UndoManager.h"
#include "cocos2d.h"

USING_NS_CC;

bool UndoManager::canUndo() const {
    return !undoStack.empty();
}

UndoRecord UndoManager::undo() {
    if (!undoStack.empty()) {
        UndoRecord record = undoStack.top();
        undoStack.pop();
        return record;
    }
    return UndoRecord{-1, MoveType::RESERVE_TO_BASE, Vec2::ZERO, -1};
}

void UndoManager::push(const UndoRecord& record) {
    undoStack.push(record);
}

void UndoManager::recordMove(const UndoRecord& record) {
    push(record);
}