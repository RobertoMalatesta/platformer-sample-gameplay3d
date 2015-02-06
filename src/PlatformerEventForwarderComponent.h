#ifndef GAME_EVENT_FORWARDER_COMPONENT_H
#define GAME_EVENT_FORWARDER_COMPONENT_H

#include "Component.h"

namespace game
{
    /**
     * Forwards relevant GameObject messages to the game
     *
     * @script{ignore}
    */
    class PlatformerEventForwarderComponent : public gameobjects::Component
    {
    public:
        explicit PlatformerEventForwarderComponent();
        ~PlatformerEventForwarderComponent();
    protected:
        virtual bool onMessageReceived(gameobjects::Message * message, int messageType) override;
    private:
        PlatformerEventForwarderComponent(PlatformerEventForwarderComponent const &);
    };
}

#endif
