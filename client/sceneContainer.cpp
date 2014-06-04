#include "sceneContainer.h"
#include <memory>
#include <iostream>
#include <cmath>

void cSceneContainer::setWorldState(const cWorldState& ws)
{
    // If an entity not owned by us exists, we set its status and that's that.
    // If it doesn't exist, whether or not it's ours, we create the dude, and thereby set its status.
    
    // But, importantly, we don't touch, at all, our own entity, if it already exists.
        
    for ( const auto& i : ws.mEntities )
    {
        if ( exists(i.mUid) )
        {
            if ( i.mUid != myOwnUid )
            {
                mEntities[i.mUid]->setStatus(i);
            }
        }
        else    // doesn't yet exist
        {
            std::cout << "Creating entity, with uid: " << i.mUid << "\n";
            std::unique_ptr<cEntity> p { new cEntity(i) };
            mEntities[i.mUid] = std::move(p);
            if ( i.mUid == myOwnUid )
            {
                mLastAgreedPlayerStatus = mEntities[i.mUid]->mStatus;   // bazzeg.
            }
        }
    }
}

bool cSceneContainer::exists(sf::Uint16 id) const
{
    return mEntities.find(id) != std::end(mEntities);
}

void cSceneContainer::savePlayerState()
{
    mPlayerLastUpdate = mEntities[myOwnUid]->mStatus.mLastUpdated;
}

void cSceneContainer::updatePlayerNow()
{
    mEntities[myOwnUid]->updateAt(mPlayerLastUpdate);
}

void cSceneContainer::partialRewindTo(sf::Int32 time)   //???
{
    mEntities[myOwnUid]->mStatus.mLastUpdated = time;
}

cWorldState& cSceneContainer::getCurrentWorldState(sf::Int32 time)
{
    mCurrentWorldState.mEntityCount = static_cast<sf::Uint16>(mEntities.size());
    mCurrentWorldState.mTime = time;
    mCurrentWorldState.mEntities.clear();
    for ( const auto& i : mEntities )
        mCurrentWorldState.mEntities.push_back(i.second->mStatus);

    return mCurrentWorldState;
}

void cSceneContainer::archiveCurrentWorldState(sf::Int32 time)
{
    mPastWorldStates[time] = getCurrentWorldState(time);
    // std::cout << "Archiving state at " << time << "\n";
}

void cSceneContainer::setPlayerInputStateAt(const cInputState& is, sf::Int32 time)
{
    mEntities[myOwnUid]->setInputStateAt(is, time);
}

void cSceneContainer::updatePlayerAt(sf::Int32 time)
{
    mEntities[myOwnUid]->updateAt(time);
}

void cSceneContainer::updateAllAt(sf::Int32 time, bool replay)
{
    for ( auto& i : mEntities )
    {
        i.second->updateAt(time);
    }
}

void cSceneContainer::updateAllWithDelta(sf::Int32 dt)
{
    for ( auto& i : mEntities )
    {
        i.second->updateWithDelta(dt);
    }
}

void cSceneContainer::updatePlayerWithoutDelta(sf::Int32 dt)
{
    mEntities[myOwnUid]->updateWithoutDelta(dt);
}

void cSceneContainer::rewindWorldTo(sf::Int32 time)
{
    if ( !mPastWorldStates.empty() && time < mPastWorldStates.rbegin()->second.mTime )
    {
        auto it = mPastWorldStates.lower_bound(time);
        if ( it != std::begin(mPastWorldStates) )
        {
            if ( it->second.mTime > time )
            {
                --it;
            }
        }
        mCurrentWorldState = it->second;
        
        setWorldState(mCurrentWorldState);
    }
}

void cSceneContainer::setPlayerStatus(const cEntityStatus& status)
{
    if ( exists(myOwnUid) )
    {
        mEntities[myOwnUid]->setStatus(status);
    }
    else
    {
        std::cout << "Creating entity, with uid: " << myOwnUid << "\n";
        std::unique_ptr<cEntity> p { new cEntity(status) };
        mEntities[myOwnUid] = std::move(p);
    }
}

void cSceneContainer::handle(const cEvent& event)
{
    switch ( event.mType ) {
        case Evt::move : {
            mEntities[event.mEntityID]->setInputStateAt(event.mInput, event.mTime);
            break;
        }
            
        case Evt::hit : {
            
            cEntityStatus tmp { event.mTargetStatus };
            tmp.mLastUpdated = event.mTime;
            mEntities[event.mEntityID]->setStatus(std::move(tmp));
            mEntities[event.mEntityID]->hit(event.mLeft, event.mTop, event.mTime);
            break;
        }
            
        case Evt::stateRefresh: {
            mPastWorldStates[event.mTime] = getCurrentWorldState(event.mTime);
            break;
        }
        default:
            break;
    }
}

void cSceneContainer::render(sf::RenderWindow& window)
{
    for ( const auto& i : mEntities )
        i.second->render(window);
}

void cSceneContainer::archiveCleanup()
{
    if ( !mPastWorldStates.empty() )
    {
        auto tBeg = mPastWorldStates.rbegin()->first - xWSArchiveLength;
        mPastWorldStates.erase(std::begin(mPastWorldStates), mPastWorldStates.lower_bound(tBeg) );
    }
}