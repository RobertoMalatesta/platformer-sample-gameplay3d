#ifndef PLATFORMER_PLAYER_MESSAGES_H
#define PLATFORMER_PLAYER_MESSAGES_H

#include "Messages.h"

namespace gameplay
{
    class AIMessage;
    class Node;
}

namespace platformer
{
    /** @script{ignore} */
    struct PlayerJumpMessage
    {
        static gameplay::AIMessage * create();
    };

    /** @script{ignore} */
    struct PlayerForceHandOfGodResetMessage
    {
        static gameplay::AIMessage * create();
    };
}

#endif
