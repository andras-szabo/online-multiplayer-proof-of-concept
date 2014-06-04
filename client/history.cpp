#include "history.h"
#include <iostream>

bool cHistory::alreadyAdded(const cEvent& event)
{
    if ( mData.find(event.mTime) != std::end(mData) )   // there's something happening at that time
    {
        if ( mData[event.mTime].mType == event.mType && mData[event.mTime].mEntityID == event.mEntityID && mData[event.mTime].mOwner == event.mOwner )
        {
            return true;
        }
        else
        {
            // There's something at event.mTime, but it's not the event we're looking for. What to do?
            // Check next event, hoping that a three-way collision would be so rare as to be negligible.
            if ( mData[event.mTime+1].mType == event.mType && mData[event.mTime+1].mEntityID == event.mEntityID && mData[event.mTime+1].mOwner == event.mOwner )
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
}

void cHistory::resetOldestEvent()
{
    mOldestEventThisTick = mYoungestEventThisTick;
    mYoungestEventThisTick = mData.rbegin()->first;
}

void cHistory::update(cEvent& event)
{
    mData[event.mTime] = event;
}

void cHistory::add(cEvent& event)
{
    
    // Find a suitable time spot: if already taken by
    // someone else, just move it 1 ms forward.
    
    sf::Int32 time = event.mTime;
    
    while ( mData.find(time) != mData.end() )
    {
        ++time; ++event.mTime;
    }
    
    mData[time] = event;
        
    if ( time < mOldestEventThisTick )
        mOldestEventThisTick = time;
    
    if ( time > mYoungestEventThisTick )
        mYoungestEventThisTick = time;
}

cEvent& cHistory::getNextEvent(sf::Int32 deadline)
{
    auto it = mData.upper_bound(deadline);
    
    if ( it == mData.end() )    // No event later than deadline
    {
        return mNullEvent;
    }
    
    return it->second;
}

void cHistory::cleanup(sf::Int32 deadline)
{
    // Deletes elements that came before deadline.
    
    mData.erase(mData.begin(), mData.lower_bound(deadline));
}