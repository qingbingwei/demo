
/**
 * @file LevelConfigLoader.cpp
 * @brief 关卡配置加载器实现，负责解析 JSON 文件生成关卡数据。
 *
 * 设计说明：
 * 1. 该模块仅负责关卡数据的解析，不涉及业务逻辑。
 * 2. 支持 Playfield、Stack、BaseStack 三种牌区的解析。
 * 3. 若 JSON 解析失败，返回空配置。
 */
#include "LevelConfigLoader.h"
#include "cocos2d.h"
#include "json/document.h"

using namespace rapidjson;
using namespace cocos2d;

/**
 * @brief 加载并解析关卡配置文件。
 * @param filename 配置文件路径。
 * @return LevelConfig 解析后的关卡配置结构体。
 * @note 若解析失败，返回空 LevelConfig。
 */
LevelConfig LevelConfigLoader::loadFromFile(const std::string& filename) {
    LevelConfig config; // 用于存储解析结果
    std::string fileData = FileUtils::getInstance()->getStringFromFile(filename); // 读取文件内容
    Document doc;
    doc.Parse(fileData.c_str()); // 解析 JSON
    if (doc.HasParseError()) {
        CCLOG("JSON failed to parse!");
        return config;
    }

    // 解析 Playfield (桌面牌区)
    if (doc.HasMember("Playfield")) {
        const auto& playfield = doc["Playfield"];
        for (rapidjson::SizeType i = 0; i < playfield.Size(); ++i) {
            CardConfig card;
            // 牌面值
            card.face = playfield[i]["CardFace"].GetInt();
            // 花色
            card.suit = playfield[i]["CardSuit"].GetInt();
            // 位置
            card.position.x = static_cast<float>(playfield[i]["Position"]["x"].GetInt());
            card.position.y = static_cast<float>(playfield[i]["Position"]["y"].GetInt());
            config.playfieldCards.push_back(card);
        }
    }

    // 解析 Stack (备用牌堆)
    if (doc.HasMember("Stack")) {
        const auto& stack = doc["Stack"];
        for (rapidjson::SizeType i = 0; i < stack.Size(); ++i) {
            CardConfig card;
            card.face = stack[i]["CardFace"].GetInt();
            card.suit = stack[i]["CardSuit"].GetInt();
            card.position.x = static_cast<float>(stack[i]["Position"]["x"].GetInt());
            card.position.y = static_cast<float>(stack[i]["Position"]["y"].GetInt());
            config.stackCards.push_back(card);
        }
    }

    // 解析 BaseStack (手牌区)
    if (doc.HasMember("BaseStack")) {
        const auto& baseStack = doc["BaseStack"];
        for (rapidjson::SizeType i = 0; i < baseStack.Size(); ++i) {
            CardConfig card;
            card.face = baseStack[i]["CardFace"].GetInt();
            card.suit = baseStack[i]["CardSuit"].GetInt();
            card.position.x = static_cast<float>(baseStack[i]["Position"]["x"].GetInt());
            card.position.y = static_cast<float>(baseStack[i]["Position"]["y"].GetInt());
            config.baseCards.push_back(card);
        }
    }

    return config;
}

// LevelConfig level = LevelConfigLoader::loadFromFile("level1.json");
// for (const auto& cardCfg : level.playfieldCards) {
//     auto card = CardView::create(cardCfg.face, cardCfg.suit, true);
//     card->setCardId(nextCardId++);
//     card->setPosition(cardCfg.position);
//     _playfieldView->addCard(card);
//     CCLOG("Playfield card: face=%d, suit=%d, pos=(%.1f,%.1f)", cardCfg.face, cardCfg.suit, cardCfg.position.x, cardCfg.position.y);
// }