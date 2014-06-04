#ifndef __clever_server_3__entity__
#define __clever_server_3__entity__

#include <SFML/Graphics.hpp>
#include "inputstate.h"

enum class EntState : sf::Uint16 { idle, walking, jumping, falling, hittingOnGround, hittingInAir, blocking, beingHit };


struct cEntityStatus {
    cInputState         mCurrentInputState;
    sf::Int32           mLastUpdated { 0 };
    sf::Uint16          mOwner { 0 };
    sf::Uint16          mUid { 0 };
    sf::Vector2f        mPos { 300, 300 };
    sf::Vector2f        mVel { 0, 0 };
    
    bool                mInAir { true };
    bool                mIsPunching { false };
    bool                mIsBlocking { false };
    
    bool                mHitAlready { false };
    
    sf::Int32           mPunchTime { 0 };           // Needed to keep track of when given punch
                                                    // started - in order not to send out duplicate
                                                    // hit events
    
    EntState            mState;
    sf::Int32           mStatePtr { 0 };          // we've been in this particular state for mStatePtr ms
    
    bool                mHitNeedsRelease { false };
};

class cEntity {
public:
    cEntity(sf::Color);
    cEntity(sf::Uint16 owner, sf::Uint16 uid, sf::Color color);
    
    void                render(sf::RenderWindow& w);
    cEntityStatus&      updateWithDelta(sf::Int32);
    cEntityStatus&      updateAt(sf::Int32);
    void                setInputStateAt(const cInputState&, sf::Int32);
    void                setStatus(const cEntityStatus&);
    void                hit(bool left, bool top, sf::Int32 time);

private:
    inline bool         canIblock() const;
    inline bool         canIpunch() const;

public:
    cEntityStatus       mStatus;
    const sf::Int32     mPunchStartup { 80 };
    const sf::Int32     mPunchActive { 280 };
    const sf::Int32     mPunchRecovery { 400 };

    const sf::Int32     mHitStun { 400 };
    
private:      
    sf::CircleShape     mShape;
    sf::Color           mColor;
    float               mAcceleration { 800.0 };
    float               mAirAcc { 400.0 };
    float               mJumpAcc { -500.0 };
    
    float               gGravity { 500 };
    float               gFriction { 20 };
    sf::Vector2f        gScreenSize { 640, 480 };
};
#endif /* defined(__clever_server_3__entity__) */
