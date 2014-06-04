#ifndef __clever_client_2__inputstate__
#define __clever_client_2__inputstate__

#include <SFML/System.hpp>

struct cInputState {
    bool        mLeft { false };
    bool        mRight { false };
    bool        mUp { false };
    bool        mPunch { false };
    bool        mBlock { false };
};

struct cInputStateChange {
    sf::Int32       mDelta;         // time difference to start of tick
    sf::Uint16      mOwner;
    sf::Uint16      mEntityID;      // so yea, which entity are we talking about?
    cInputState     mInput;         // and what's the actual input
};

struct cISC {
    cInputStateChange       mItem[20];          // should be more than enough
    sf::Uint16              mItemCount { 0 };
    const sf::Uint16        mMaxItems { 20 };
    sf::Uint16              mIscCount[4];       // this many state changes for each of the players
};

#endif /* defined(__clever_client_2__inputstate__) */
