#ifndef __clever_client_4__sceneContainer__
#define __clever_client_4__sceneContainer__

#include <map>
#include <memory>
#include "entity.h"
#include "worldState.h"
#include "history.h"
#include "inputstate.h"
#include "event.h"
#include <SFML/Graphics.hpp>
#include <queue>

class cSceneContainer {
public:
    void                setWorldState(const cWorldState&);
    void                partialRewindTo(sf::Int32);

    void                savePlayerState();
    void                updatePlayerNow();
    void                updatePlayerAt(sf::Int32);
    void                updateAllWithDelta(sf::Int32);
    void                updateAllAt(sf::Int32, bool replay = false);
    void                updatePlayerWithoutDelta(sf::Int32);
    
    void                setPlayerInputStateAt(const cInputState&, sf::Int32);
    cWorldState&        getCurrentWorldState(sf::Int32);
    void                archiveCurrentWorldState(sf::Int32);
    
    void                rewindWorldTo(sf::Int32);
    void                setPlayerStatus(const cEntityStatus&);
    void                handle(const cEvent&);

    void                render(sf::RenderWindow&);
    void                archiveCleanup();
        
private:
    inline bool         exists(sf::Uint16) const;
    
public:
    sf::Uint16          myOwnUid { 0 };
    cEntityStatus       mLastAgreedPlayerStatus;
    cWorldState         mCurrentWorldState;
    sf::Int32           mLastAgreedCommandTime { 0 };
    
private:
    std::map<sf::Uint16, std::unique_ptr<cEntity>>  mEntities;
    sf::Int32                                       mPlayerLastUpdate;
    std::map<sf::Int32, cWorldState>                mPastWorldStates;

    
    const sf::Int32     xWSArchiveLength { 2000 };  // in MS
};

#endif /* defined(__clever_client_4__sceneContainer__) */
