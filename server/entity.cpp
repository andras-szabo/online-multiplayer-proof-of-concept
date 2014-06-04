#include "entity.h"
#include <iostream>

cEntity::cEntity(sf::Color col)
{
    mColor = col;
    mShape.setFillColor(mColor);
    mShape.setRadius(40);
    mShape.setOrigin(40, 40);
}

cEntity::cEntity(sf::Uint16 owner,
                 sf::Uint16 uid,
                 sf::Color col)
{
    mColor = col;
    mShape.setFillColor(col);
    mShape.setRadius(40);
    mShape.setOrigin(40, 40);
    
    mStatus.mOwner = owner;
    mStatus.mUid = uid;
}

void cEntity::render(sf::RenderWindow& w)
{
    mShape.setPosition(mStatus.mPos);
    
    sf::Color col;
    
    switch ( mStatus.mState )
    {
        case EntState::idle: col = sf::Color::Green; break;
        case EntState::walking: col = sf::Color::Blue; break;
        case EntState::jumping: col = sf::Color::Yellow; break;
        case EntState::falling: col = sf::Color::Cyan; break;
        case EntState::blocking: col = sf::Color::White; break;
        case EntState::beingHit: col = sf::Color::Magenta; break;
        case EntState::hittingInAir: col = sf::Color::Green; break;
        case EntState::hittingOnGround: col = sf::Color::Red; break;
    }
    
    mShape.setFillColor(col);
    w.draw(mShape);
}

void cEntity::setInputStateAt(const cInputState& in, sf::Int32 time)
{
    updateWithDelta(time - mStatus.mLastUpdated);   // update with current state, adjust counter
    mStatus.mCurrentInputState = in;
}

void cEntity::hit(bool left, bool top, sf::Int32 time)
{
    if ( mStatus.mState == EntState::beingHit )
    {
        // We're already being hit!
        return;
    }

    updateWithDelta(time - mStatus.mLastUpdated);   // update with current state, adjust counter
    
    mStatus.mVel.x += ( left == true ? -300 : 300);
    if ( top )
    {
        if ( mStatus.mInAir )
        {
            mStatus.mVel.y += 200;
        }
    }
    else
    {
        mStatus.mVel.y -= 200;
        mStatus.mInAir = true;
    }
    
    mStatus.mLastUpdated = time;
    mStatus.mState = EntState::beingHit;
    mStatus.mStatePtr = 0;
}

bool cEntity::canIblock() const
{
    return !mStatus.mInAir && !mStatus.mIsPunching && ( mStatus.mState != EntState::beingHit );
}

bool cEntity::canIpunch() const
{
    return !mStatus.mIsPunching && !mStatus.mHitNeedsRelease && ( mStatus.mState != EntState::beingHit );
}

cEntityStatus& cEntity::updateWithDelta(sf::Int32 dt)
{
    auto delta = static_cast<float>(dt) / 1000.0;
    
    // First thing first: apply last velocity
    
    mStatus.mPos.x += ( mStatus.mVel.x * delta);
    mStatus.mPos.y += ( mStatus.mVel.y * delta);
    
    // OK, now calculate new velocity on the basis of where we are,
    // and what the inputstate is.
    
    // mStatus.mInAir = areWeInAir();
    // for now:
    
    mStatus.mInAir = ( mStatus.mPos.y < gScreenSize.y - 125 );
    mStatus.mVel.y += ( mStatus.mInAir * (gGravity * delta));

    mStatus.mIsBlocking = mStatus.mCurrentInputState.mBlock;
    
    if ( mStatus.mHitNeedsRelease && mStatus.mCurrentInputState.mPunch == false )
    {
        mStatus.mHitNeedsRelease = false;
    }
    
    if ( mStatus.mInAir )
    {
        mStatus.mVel.x += mStatus.mCurrentInputState.mRight * ( mAirAcc * delta );
        mStatus.mVel.x -= mStatus.mCurrentInputState.mLeft * ( mAirAcc * delta );
    }
    else
    {
        if ( !mStatus.mIsBlocking &&
             (mStatus.mCurrentInputState.mRight || mStatus.mCurrentInputState.mLeft ) )
        {
            mStatus.mVel.x += mStatus.mCurrentInputState.mRight * ( mAirAcc * delta );
            mStatus.mVel.x -= mStatus.mCurrentInputState.mLeft * ( mAirAcc * delta );
        }
        else
        {
            mStatus.mVel.x *= 1 - ( gFriction * delta );
            if ( abs(mStatus.mVel.x) < 5 ) mStatus.mVel.x = 0;
        }
        
        if ( mStatus.mCurrentInputState.mUp && !mStatus.mIsBlocking )
        {
            // After jump, reset jump input, because this won't be sent by
            // the client
            
            mStatus.mVel.y += mJumpAcc;
            mStatus.mInAir = true;
            mStatus.mCurrentInputState.mUp = false;
        }
    }

    // Keep velocities, position in check:
    
    if ( mStatus.mVel.x > 800 ) mStatus.mVel.x = 800;
    if ( mStatus.mVel.x < -800 ) mStatus.mVel.x = -800;

    if ( mStatus.mVel.y > 800 ) mStatus.mVel.y = 800;
    if ( mStatus.mVel.y < -800 ) mStatus.mVel.y = -800;
    
    if ( mStatus.mPos.x < 0 ) { mStatus.mPos.x = 2; mStatus.mVel.x = 0; }
    if ( mStatus.mPos.x > gScreenSize.x ) { mStatus.mPos.x = gScreenSize.x - 2; mStatus.mVel.x = 0; }
    
    if ( mStatus.mPos.y < 0 ) { mStatus.mPos.y = 2; mStatus.mVel.y = 0; }
    if ( mStatus.mPos.y > gScreenSize.y - 125 )
    {
        mStatus.mPos.y = gScreenSize.y - 125;
        mStatus.mVel.y = 0;
    };
    
    // Now: gravity, friction, movement covered. OK. now what about punching or blocking?

    // Blocking:
    // - if not blocking, but in a state where it's possible to block, then switch to blocking.
    // - if already blocking, do nothing
    
    if ( mStatus.mCurrentInputState.mBlock && canIblock())
    {
        mStatus.mIsPunching = false;
        mStatus.mIsBlocking = true;
        mStatus.mState = EntState::blocking;
        mStatus.mStatePtr = 0;
    }
    
    // Hitting others.
    // if not hitting already, and it's possible to hit - hit 'em.
    
    if ( mStatus.mCurrentInputState.mPunch && canIpunch())
    {
        mStatus.mIsPunching = true;
        mStatus.mIsBlocking = false;
        mStatus.mHitAlready = false;
        
        mStatus.mHitNeedsRelease = true;

        mStatus.mPunchTime = mStatus.mLastUpdated + dt;
        
        if ( mStatus.mInAir )
        {
            mStatus.mState = EntState::hittingInAir;
        }
        else
        {
            mStatus.mState = EntState::hittingOnGround;
        }
        
        mStatus.mStatePtr = 0;
    }
    
    // OK, now, if we're hitting or blocking, we're done; if not, let's see if
    // we're jumping, falling, idling, or walking ( or being hit )
    
    if ( !mStatus.mIsBlocking && !mStatus.mIsPunching && mStatus.mState != EntState::beingHit )
    {
        EntState newState;
        if ( mStatus.mInAir && mStatus.mVel.y < 0 ) newState = EntState::jumping;
        if ( mStatus.mInAir && mStatus.mVel.y >=0 ) newState = EntState::falling;
        if ( !mStatus.mInAir && mStatus.mVel.x == 0 ) newState = EntState::idle;
        if ( !mStatus.mInAir && mStatus.mVel.x != 0 ) newState = EntState::walking;
        
        if ( mStatus.mState != newState )
        {
            mStatus.mState = newState;
            mStatus.mStatePtr = 0;
        }
    }
    
    mStatus.mStatePtr += dt;
    
    if ((mStatus.mStatePtr > mPunchRecovery &&
        (mStatus.mState == EntState::hittingInAir || mStatus.mState == EntState::hittingOnGround ))
        ||
        (mStatus.mStatePtr > mHitStun && mStatus.mState == EntState::beingHit)
        )
    {
        if ( mStatus.mInAir && mStatus.mVel.y < 0 ) mStatus.mState = EntState::jumping;
        if ( mStatus.mInAir && mStatus.mVel.y >=0 ) mStatus.mState = EntState::falling;
        if ( !mStatus.mInAir && mStatus.mVel.x == 0 ) mStatus.mState = EntState::idle;
        if ( !mStatus.mInAir && mStatus.mVel.x != 0 ) mStatus.mState = EntState::walking;
        
        mStatus.mStatePtr = 0;
        mStatus.mIsPunching = false;
        mStatus.mIsBlocking = false;

        mStatus.mCurrentInputState.mPunch = false;
    }
    
    mStatus.mLastUpdated += dt;
    return mStatus;
}

cEntityStatus& cEntity::updateAt(sf::Int32 time)
{
    updateWithDelta(time - mStatus.mLastUpdated);
    return mStatus;
}

void cEntity::setStatus(const cEntityStatus& st)
{
    mStatus = st;
}