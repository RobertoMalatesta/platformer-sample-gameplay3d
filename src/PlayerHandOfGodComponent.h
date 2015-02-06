#ifndef GAME_PLAYER_HOG_COMPONENT_H
#define GAME_PLAYER_HOG_COMPONENT_H

#include "Component.h"

namespace game
{
    class CameraControlComponent;
    class PlayerComponent;

    /**
     * Moves the player to the spawn position when they venture outside the boundaries of the current level
     *
     * @script{ignore}
    */
    class PlayerHandOfGodComponent : public gameobjects::Component
    {
    public:
        explicit PlayerHandOfGodComponent();
        ~PlayerHandOfGodComponent();
    protected:
        virtual void update(float elapsedTime) override;
        virtual void initialize() override;
        virtual void finalize() override;
        virtual void onStart() override;
        virtual bool onMessageReceived(gameobjects::Message * message, int messageType) override;
    private:
        gameplay::Vector2 _resetPosition;
        gameplay::Rectangle _boundary;
        PlayerComponent * _player;
        CameraControlComponent * _camera;
        bool _levelLoaded;
        bool _forceNextReset;
        gameobjects::Message * _splashScreenFadeMessage;
    };
}

#endif
