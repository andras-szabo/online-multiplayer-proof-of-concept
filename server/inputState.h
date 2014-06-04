#ifndef __clever_server_3__inputState__
#define __clever_server_3__inputState__

#include <SFML/System.hpp>
#include <map>

struct cInputState {
    bool        mLeft { false };
    bool        mRight { false };
    bool        mUp { false };
    bool        mPunch { false };
    bool        mBlock { false };
};

struct cInputStateChange {
    sf::Int32       mDelta;         // time difference to start of tick
    sf::Uint16      mOwner;         // so yea, whose entity are we talking about?
    sf::Uint16      mEntityID;      // which entity does this refer to
    cInputState     mInput;         // and what's the actual input
};

struct cISC {

                    sf::Uint16          changesForOthers(sf::Uint16) const;
    
    cInputStateChange                   mItem[20];          // should be more than enough
    sf::Uint16                          mItemCount { 0 };
    const sf::Uint16                    mMaxItems { 20 };
    std::map<sf::Uint16, sf::Uint16>    mIscCount;
};

#endif /* defined(__clever_server_3__inputState__) */
