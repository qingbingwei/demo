#pragma once
struct CardModel {
    int id;         // 唯一ID
    int face;       // 点数
    int suit;       // 花色
    bool isFaceUp;  // 是否翻开
    bool isRemoved; // 是否已消除
    float posX, posY; // 位置
};