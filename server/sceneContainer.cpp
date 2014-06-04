#include "sceneContainer.h"
#include <iostream>
#include <cmath>

sf::Uint16 cSceneContainer::add(sf::Uint16 owner, sf::Color color)
{
    std::unique_ptr<cEntity> p { new cEntity { owner, mNextAvailableUid, color } };
    mEntities[mNextAvailableUid] = std::move(p);

    ++mCurrentWorldState.mEntityCount;
    mCurrentWorldState.mEntities.push_back(mEntities[mNextAvailableUid]->mStatus);
    
    return mNextAvailableUid++;
}

void cSceneContainer::updateWorldAt(sf::Int32 time)
{
    // So, iterate thru entities; update each, put the status
    // returned into mCurrentWorldState.
    
    for ( auto& i : mCurrentWorldState.mEntities )
        i = mEntities[i.mUid]->updateAt(time);
    
    // Check for collisions; for now: brute force, everyone with everyone,
    // but nobody with itself
    
    for ( auto& i : mCurrentWorldState.mEntities )
    {
        if ( i.mIsPunching && i.mStatePtr > mEntities[i.mUid]->mPunchStartup &&
            i.mStatePtr < mEntities[i.mUid]->mPunchActive &&
            i.mHitAlready == false )
        {
            for ( auto& j : mCurrentWorldState.mEntities )
            {
                if ( i.mUid != j.mUid && j.mState != EntState::blocking )
                 {
                     if ( abs(i.mPos.x - j.mPos.x) < 80 && abs(i.mPos.y - j.mPos.y) < 80 )
                     {
                         double dx = i.mPos.x - j.mPos.x;
                         double dy = i.mPos.y - j.mPos.y;
                         if ( sqrt(pow(dx,2) + pow(dy,2)) < 80 )
                         {
                             
                             // If this punch has not been registered already,
                             // then we actually send the hit event; this
                             // is something new.
                             
                             auto it = mHitRecords.find(i.mPunchTime);
                             
                             bool multipleHit { false };
                             
                             if ( it != std::end(mHitRecords) ) // punch already there
                             {
                                 auto jt = std::find(std::begin(it->second.mEntityID),
                                                     std::end(it->second.mEntityID),
                                                     j.mUid);
                                 
                                 // If we can't find the currently hit entity's ID in the
                                 // hit records, then it's a hit that hit multiple targets.
                                 
                                 if ( jt == std::end(it->second.mEntityID) )
                                 {
                                     // but not yet registered with this punchtime
                                     multipleHit = true;
                                 }
                             }
                             
                             if ( it == std::end(mHitRecords) || multipleHit )
                             {
                                 
                                 // So if either this hit is not in the hit records, or the geezer hit
                                 // is one of several targets and has not yet been entered into hit records,
                                 // then we need to create a new hit event and send it to all the clients
                                 
                                 if ( multipleHit )
                                 {
                                     // Expanding hit records
                                     mHitRecords[i.mPunchTime].mEntityID.push_back(j.mUid);
                                 }
                                 else
                                 {
                                     // Creating new hit record
                                     cHitRecord tmp;
                                     tmp.mOwner = i.mUid;
                                     tmp.mEntityID.push_back(j.mUid);
                                     mHitRecords[i.mPunchTime] = std::move(tmp);
                                 }
                                 
                                 cEvent hitEvent { Evt::hit };
                                 
                                 hitEvent.mOwner = i.mUid;
                                 hitEvent.mEntityID = j.mUid;
                                 hitEvent.mLeft = dx > 0;
                                 hitEvent.mTop = dy > 0;
                                 hitEvent.mTime = time;
                                 hitEvent.mTargetStatus = j;
                                 
                                 mHitQueue.push_back(std::move(hitEvent));
                                 
                                 // Resolving hit now - but only if the hit is as yet unregistered;
                                 // otherwise it was processed by a hit event in mHistory (or it will
                                 // be, if it was not yet).
                                 
                                 mEntities[j.mUid]->hit(dx > 0, dy > 0, time);
                                 j = mEntities[j.mUid]->mStatus;
                                 
                                 i.mHitAlready = true;
                                 mEntities[i.mUid]->mStatus.mHitAlready = true;
                                 
                             }
                             else
                             {
                                 // Hit, but this has already been registered, so let's NOT resend it.
                             }
                         }
                     }
                 }
             }
        }
    }
    
    mCurrentWorldState.mTime = time;
}

void cSceneContainer::updateArchiveAt(sf::Int32 time)
{
    mPastWorldStates[time] = mCurrentWorldState;
}

void cSceneContainer::rewindWorldTo(sf::Int32 time)
{
    auto it = mPastWorldStates.lower_bound(time);
    --it;
    mCurrentWorldState = it->second;
    
    // Now let's have everyone adapt to the new world state
    
    for ( auto& i : mCurrentWorldState.mEntities )
        mEntities[i.mUid]->setStatus(i);
}

cEntityStatus cSceneContainer::getStatus(sf::Uint16 id)
{
    return mEntities[id]->mStatus;
}

void cSceneContainer::updatePrevWorldState()
{
    if ( mPastWorldStates.size() > 0 )
    {
        mPreviousWorldState = mPastWorldStates.rbegin()->second;
    }
}

void cSceneContainer::saveCurrent(sf::Int32 time)
{
    mPastWorldStates[time] = mCurrentWorldState;
}

void cSceneContainer::render(sf::RenderWindow& w)
{
    for ( const auto& i : mEntities )
        i.second->render(w);
}

void cSceneContainer::cleanup(sf::Int32 time)
{
    
    // Clean up past world state archives
    
    mPastWorldStates.erase(std::begin(mPastWorldStates),
                           mPastWorldStates.lower_bound(time));
    
    // Clean up hit record archives
    
    mHitRecords.erase(std::begin(mHitRecords), mHitRecords.lower_bound(time));
}

void cSceneContainer::moveEntity(sf::Uint16 uid, const cInputState& input, sf::Int32 time)
{
    mEntities[uid]->setInputStateAt(input, time);
}

void cSceneContainer::hitEntity(const cEvent& hitevent)
{
    mEntities[hitevent.mEntityID]->hit(hitevent.mLeft, hitevent.mTop, hitevent.mTime);
}