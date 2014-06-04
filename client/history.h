#ifndef __clever_server_3__history__
#define __clever_server_3__history__

#include <map>
#include <SFML/System.hpp>
#include "event.h"

class cHistory {
public:
    void        add(cEvent&);
    void        update(cEvent&);
    void        cleanup(sf::Int32);
    cEvent&     getNextEvent(sf::Int32);
    void        resetOldestEvent();
    bool        alreadyAdded(const cEvent&);
    
public:
    sf::Int32   mOldestEventThisTick { 0 };
    sf::Int32   mYoungestEventThisTick { 0 };
    
private:
    std::map<sf::Int32, cEvent>     mData;
    cEvent                          mNullEvent;
};
#endif /* defined(__clever_server_3__history__) */
