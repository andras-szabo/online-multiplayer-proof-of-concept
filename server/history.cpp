#include "history.h"

void cHistory::resetOldestEvent()
{
    mOldestEventThisTick = mYoungestEventThisTick;
    mYoungestEventThisTick = mData.rbegin()->first;
}

void cHistory::update(sf::Int32 time, cEvent& event)
{
    mData[time] = event;
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
    
    if ( it->second.mTime == deadline )
        throw std::runtime_error("Call Mulder and Scully.");
    
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