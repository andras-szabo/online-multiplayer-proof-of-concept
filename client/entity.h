#ifndef __clever_client_2__entity__
#define __clever_client_2__entity__

#include <SFML/Graphics.hpp>
#include "ResourcePath.hpp"
#include "inputstate.h"

enum class EntState : sf::Uint16 { idle, walking, jumping, falling, hittingOnGround, hittingInAir, blocking, beingHit };

struct cAnim {
    sf::IntRect         mTexRect;
    sf::Vector2f        mCentre;
    sf::Uint8           mPhaseCount { 0 };
};

struct cEntityStatus {
    cInputState         mCurrentInputState;
    sf::Int32           mLastUpdated { 0 };
    sf::Uint16          mOwner { 0 };
    sf::Uint16          mUid { 0 };
    sf::Vector2f        mPos { 300, 300 };
    sf::Vector2f        mVel { 0, 0 };
    
    EntState            mState;
    sf::Int32           mStatePtr { 0 };

    bool                mInAir { true };
    bool                mIsPunching { false };
    bool                mIsBlocking { false };
    bool                mHitAlready { false };      // did we hit anyone with this particular punch?
                                                    // if so, we must not hit again - i.e. one hit should
                                                    // register as one hit, not multiple hits

    bool                mHitNeedsRelease { false }; // New hit event can only register if this is false.
};


class cEntity {
public:
    cEntity(sf::Color);
    cEntity(const cEntityStatus&);
    
    void                render(sf::RenderWindow& w);
    cEntityStatus&      updateWithDelta(sf::Int32);
    cEntityStatus&      updateAt(sf::Int32);
    cEntityStatus&      updateWithoutDelta(sf::Int32);
    void                setInputStateAt(const cInputState&, sf::Int32);
    void                setStatus(const cEntityStatus&);
    void                hit(bool left, bool top, sf::Int32 time);

private:
    void                setUpAnimations();
    inline bool         canIblock() const;
    inline bool         canIpunch() const;

public:
    cEntityStatus       mStatus;
    sf::Uint16          mOwner;
    sf::Uint16          mUid;
    
    const sf::Int32     mPunchStartup { 80 };
    const sf::Int32     mPunchActive { 280 };
    const sf::Int32     mPunchRecovery { 400 };
    const sf::Int32     mHitStun { 400 };

    const sf::Int16     gAnimFPS { 100 };   // it's more like: milliseconds-per-frame
    
private:
    sf::Texture                     mTexture;
    sf::Sprite                      mSprite;
    std::map<EntState, cAnim>       mAnim;
    
    sf::CircleShape     mShape;
    sf::Color           mColor;
    float               mAcceleration { 800.0 };
    float               mJumpAcc { -500.0 };
    float               mAirAcc { 400 };
    
    float               gGravity { 500 };
    float               gFriction { 20 };
    sf::Vector2f        gScreenSize { 640, 480 };
    
    bool                mFacingRight { true };
    bool                mPrevFacingRight { true };
};

#endif /* defined(__clever_client_2__entity__) */
