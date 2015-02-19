#ifndef GAME_H
#define GAME_H

#include "Control.h"
#include "Game.h"
#include "GameObjectMessage.h"

namespace game
{
    /**
     * A 2D platformer
     *
     * @script{ignore}
    */
    class Platformer : public gameplay::Game, public gameplay::Control::Listener
    {
    public:
        explicit Platformer();
        virtual ~Platformer();

    protected:
        virtual void initialize() override;
        virtual void finalize() override;
        virtual void gesturePinchEvent(int x, int y, float scale) override;
        virtual void keyEvent(gameplay::Keyboard::KeyEvent evt, int key) override;
        virtual void touchEvent(gameplay::Touch::TouchEvent evt, int x, int y, unsigned int contactIndex) override;
        virtual bool mouseEvent(gameplay::Mouse::MouseEvent evt, int x, int y, int wheelDelta) override;
        virtual void update(float elapsedTime) override;
        virtual void render(float elapsedTime) override;
        virtual void controlEvent(gameplay::Control * control, gameplay::Control::Listener::EventType);
    private:
        void broadcastKeyEvent(gameplay::Keyboard::KeyEvent evt, int key);

        gameobjects::Message * _pinchMessage;
        gameobjects::Message * _keyMessage;
        gameobjects::Message * _touchMessage;
        gameobjects::Message * _mouseMessage;
        gameobjects::Message * _UpdateAndRenderLevelMessage;
        gameplay::Form * _optionsForm;
#ifndef WIN32
        int _previousReleasedKey;
        int _framesSinceKeyReleaseEvent;
#endif
    };
}

#endif
