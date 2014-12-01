#ifndef PLATFORMER_GAME_MESSAGES_H
#define PLATFORMER_GAME_MESSAGES_H

#include "Messages.h"

namespace gameplay
{
    class AIMessage;
}

namespace platformer
{
    /** @script{ignore} */
    struct PlatformerSplashScreenChangeRequestMessage
    {
        PlatformerSplashScreenChangeRequestMessage(gameplay::AIMessage * message);
        static gameplay::AIMessage * create();
        static void setMessage(gameplay::AIMessage * message, float duration, bool isFadingIn);

        float _duration;
        bool _isFadingIn;
    };
}

#endif
