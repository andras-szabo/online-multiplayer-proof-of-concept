#ifndef __clever_client_2__worldState__
#define __clever_client_2__worldState__

#include <SFML/System.hpp>
#include "entity.h"
#include <list>

struct cWorldState {
    void    updateTime(sf::Int32);
    
    sf::Uint16                  mEntityCount { 0 };
    std::list<cEntityStatus>    mEntities;
    sf::Int32                   mTime { 0 };
};
#endif /* defined(__clever_client_2__worldState__) */
