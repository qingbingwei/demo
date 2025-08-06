#pragma once
#include "cocos2d.h"
#include <vector>
#include <map>
#include "CardView.h"
#include <functional>

/**
 * @brief Stores card properties for undo/restore operations
 */
struct CardRestoreInfo {
    int cardId;
    cocos2d::Vec2 position;
    int zOrder;
    bool visible;
};

/**
 * @class PlayfieldView
 * @brief Manages the game's main play area where cards are displayed and interacted with
 *
 * PlayfieldView is responsible for:
 * - Managing card display, positioning, and z-order in the main play area
 * - Handling card overlap detection to determine if a card can be clicked
 * - Supporting different layout strategies for arranging cards
 * - Providing undo functionality through card state saving/restoring
 */
class PlayfieldView : public cocos2d::Node {
public:
    /**
     * @brief Layout strategies for arranging cards in the playfield
     */
    enum class LayoutType {
        KEEP_ORIGINAL,    ///< Maintain original card positions
        HORIZONTAL_LINE,  ///< Arrange cards in a horizontal line
        CUSTOM           ///< Custom layout defined by application
    };

    /**
     * @brief Factory method to create a PlayfieldView instance
     * @return A newly created PlayfieldView instance
     */
    static PlayfieldView* create();
    
    /**
     * @brief Initialize the PlayfieldView
     * @return true if initialization was successful
     */
    virtual bool init();
    
    /**
     * @brief Add a card to the playfield
     * @param cardView The card to add
     */
    void addCard(CardView* cardView);
    
    /**
     * @brief Remove a card from the playfield
     * @param cardView The card to remove
     */
    void removeCard(CardView* cardView);
    
    /**
     * @brief Set the callback function for card click events
     * @param callback The function to call when a card is clicked
     */
    void setOnCardClickCallback(const std::function<void(int)>& callback);
    
    /**
     * @brief Arrange cards according to the specified layout type
     * @param type The layout strategy to use
     */
    void layoutCards(LayoutType type = LayoutType::KEEP_ORIGINAL);
    
    /**
     * @brief Get all cards in the playfield
     * @return A reference to the vector of cards
     */
    const std::vector<CardView*>& getCards() const;
    
    /**
     * @brief Check if a card is covered by other cards
     * @param targetCard The card to check
     * @return true if the card is covered, false otherwise
     */
    bool isCardCovered(CardView* targetCard) const;
    
    /**
     * @brief Save a card's current state for later restoration
     * @param cardId The ID of the card to save
     */
    void saveCardState(int cardId);
    
    /**
     * @brief Restore a card to its previously saved state
     * @param cardId The ID of the card to restore
     */
    void restoreCardState(int cardId);
    
    // Constants for playfield dimensions
    static const int PLAYFIELD_WIDTH = 1080;
    static const int PLAYFIELD_HEIGHT = 1500;

private:
    /**
     * @brief Find a card by its ID
     * @param cardId The ID of the card to find
     * @return Pointer to the card or nullptr if not found
     */
    CardView* findCardById(int cardId) const;
    
    /**
     * @brief Set up the z-order for a card
     * @param cardView The card to set up
     * @param originalZOrder The original z-order value
     */
    void setupCardZOrder(CardView* cardView, int originalZOrder);
    
    /**
     * @brief Find the highest z-order among all cards
     * @return The highest z-order value
     */
    int findHighestZOrder() const;
    
    /**
     * @brief Set up the visibility state for a card
     * @param cardView The card to set up
     * @param wasVisible Whether the card was previously visible
     */
    void setupCardVisibility(CardView* cardView, bool wasVisible);
    
    /**
     * @brief Set up the click callback for a card
     * @param cardView The card to set up
     */
    void setupCardClickCallback(CardView* cardView);
    
    /**
     * @brief Get the effective size of a card, accounting for safety margins
     * @param cardView The card to measure
     * @return The effective size
     */
    cocos2d::Size getEffectiveCardSize(CardView* cardView) const;
    
    /**
     * @brief Check if there are any cards overlapping with the target card
     * @param targetCard The card to check
     * @param targetPos The position of the target card
     * @param targetSize The size of the target card
     * @param targetZOrder The z-order of the target card
     * @return true if there's an overlapping card, false otherwise
     */
    bool checkForOverlappingCards(CardView* targetCard, 
                                 const cocos2d::Vec2& targetPos,
                                 const cocos2d::Size& targetSize,
                                 int targetZOrder) const;
    
    /**
     * @brief Check if two cards overlap
     * @param targetCard The first card
     * @param otherCard The second card
     * @param left1 The left edge of the first card's bounding box
     * @param right1 The right edge of the first card's bounding box
     * @param bottom1 The bottom edge of the first card's bounding box
     * @param top1 The top edge of the first card's bounding box
     * @return true if the cards overlap, false otherwise
     */
    bool cardsOverlap(CardView* targetCard, 
                     CardView* otherCard,
                     float left1, float right1, 
                     float bottom1, float top1) const;
    
    /**
     * @brief Layout cards using the keep original strategy
     */
    void layoutKeepOriginal();
    
    /**
     * @brief Layout cards using the horizontal line strategy
     */
    void layoutHorizontalLine();
    
    /**
     * @brief Layout cards using the custom strategy
     */
    void layoutCustom();
    
    // Member variables
    std::vector<CardView*> _cards;               ///< Collection of cards in the playfield
    std::map<int, CardRestoreInfo> _cardStates;  ///< Saved card states for undo operations
    std::function<void(int)> _onCardClickCallback; ///< Callback for card click events
    LayoutType _currentLayout{LayoutType::KEEP_ORIGINAL}; ///< Current layout strategy
};

