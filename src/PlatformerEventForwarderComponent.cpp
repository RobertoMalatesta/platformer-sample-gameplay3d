#include "PlatformerEventForwarderComponent.h"

#include "Messages.h"
#include "Platformer.h"

namespace platformer
{
    PlatformerEventForwarderComponent::PlatformerEventForwarderComponent()
    {
    }

    PlatformerEventForwarderComponent::~PlatformerEventForwarderComponent()
    {
    }

    void PlatformerEventForwarderComponent::onMessageReceived(gameplay::AIMessage * message)
    {
        Platformer * game = static_cast<Platformer*>(gameplay::Game::getInstance());

        switch (message->getId())
        {
            case (Messages::Type::PlatformerSplashScreenChangeRequest):
            {
                PlatformerSplashScreenChangeRequestMessage msg(message);
                game->setSplashScreenFade(msg._duration, msg._isFadingIn);
            }
            break;
        }
    }
}
