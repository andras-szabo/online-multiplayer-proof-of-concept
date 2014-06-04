#ifndef __clever_server_3__worldState__
#define __clever_server_3__worldState__

#include <SFML/System.hpp>
#include "entity.h"
#include <list>

struct cWorldState {
    sf::Uint16                  mEntityCount { 0 };
    std::list<cEntityStatus>    mEntities;
    sf::Int32                   mTime { 0 };
};


#endif /* defined(__clever_server_3__worldState__) */
