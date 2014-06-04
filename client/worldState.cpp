#include "worldState.h"

void cWorldState::updateTime(sf::Int32 time)
{
    mTime = time;
    for ( auto& i : mEntities )
        i.mLastUpdated = time;
}
