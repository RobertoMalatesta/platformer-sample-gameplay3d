#ifndef PLATFORMER_EVENT_FORWARDER_COMPONENT_H
#define PLATFORMER_EVENT_FORWARDER_COMPONENT_H

#include "Component.h"

namespace platformer
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
        virtual void onMessageReceived(gameobjects::GameObjectMessage message, int messageType) override;
    private:
        PlatformerEventForwarderComponent(PlatformerEventForwarderComponent const &);
    };
}

#endif
