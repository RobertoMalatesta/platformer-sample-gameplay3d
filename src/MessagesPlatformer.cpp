#include "MessagesPlatformer.h"

#include "gameplay.h"

namespace platformer
{
    /** @script{ignore} */
    struct PlatformerSplashScreenChangeRequestMessageArgs
    {
        enum Enum
        {
            IsFadingIn,
            Duration,

            ArgCount
        };
    };

    PlatformerSplashScreenChangeRequestMessage::PlatformerSplashScreenChangeRequestMessage(gameplay::AIMessage * message)
        : _duration(0.0f)
        , _isFadingIn(true)
    {
        GP_ASSERT(message->getId() == Messages::Type::PlatformerSplashScreenChangeRequestMessage);
        _duration = message->getFloat(PlatformerSplashScreenChangeRequestMessageArgs::Duration);
        _isFadingIn = message->getBoolean(PlatformerSplashScreenChangeRequestMessageArgs::IsFadingIn);
    }

    gameplay::AIMessage * PlatformerSplashScreenChangeRequestMessage::create()
    {
        return gameplay::AIMessage::create(Messages::Type::PlatformerSplashScreenChangeRequestMessage, "", "", PlatformerSplashScreenChangeRequestMessageArgs::ArgCount);
    }

    void PlatformerSplashScreenChangeRequestMessage::setMessage(gameplay::AIMessage * message, float duration, bool isFadingIn)
    {
        GP_ASSERT(message->getId() == Messages::Type::PlatformerSplashScreenChangeRequestMessage);
        message->setFloat(PlatformerSplashScreenChangeRequestMessageArgs::Duration, duration);
        message->setBoolean(PlatformerSplashScreenChangeRequestMessageArgs::IsFadingIn, isFadingIn);
    }
}
