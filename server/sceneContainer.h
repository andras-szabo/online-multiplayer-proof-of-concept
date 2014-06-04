#ifndef __clever_server_4__sceneContainer__
#define __clever_server_4__sceneContainer__

#include <SFML/Graphics.hpp>
#include "entity.h"
#include "worldState.h"
#include "event.h"

struct cHitRecord {
    sf::Uint16                  mOwner;     // the one that punched
    std::vector<sf::Uint16>     mEntityID;  // the one(s) that got hit
};

class cSceneContainer {
public:
    sf::Uint16      add(sf::Uint16 owner, sf::Color color); // add entity for given client; return entity ID
    void            updateWorldAt(sf::Int32);
    void            rewindWorldTo(sf::Int32);
    void            updateArchiveAt(sf::Int32);
    cEntityStatus   getStatus(sf::Uint16);
    void            updatePrevWorldState();
    void            saveCurrent(sf::Int32);
    void            render(sf::RenderWindow&);
    void            cleanup(sf::Int32);
    void            moveEntity(sf::Uint16, const cInputState&, sf::Int32);
    void            hitEntity(const cEvent& hitevent);
    
public:
    cWorldState                         mCurrentWorldState;
    cWorldState                         mPreviousWorldState;
    std::map<sf::Int32, cWorldState>    mPastWorldStates;
    std::vector<cEvent>                 mHitQueue;
    
private:
    std::map<sf::Uint16, std::unique_ptr<cEntity>>  mEntities;
    std::map<sf::Int32, cHitRecord>                 mHitRecords;
    sf::Uint16      mNextAvailableUid { 1 };
};

#endif /* defined(__clever_server_4__sceneContainer__) */