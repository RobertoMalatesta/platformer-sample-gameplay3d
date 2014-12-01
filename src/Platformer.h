#ifndef PLATFORMER_H
#define PLATFORMER_H

#include "gameplay.h"

namespace platformer
{
    class SpriteSheet;

    /**
     * A 2D platformer
     *
     * @script{ignore}
    */
    class Platformer : public gameplay::Game
    {
        friend class PlatformerEventForwarderComponent;

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
        virtual void gamepadEvent(gameplay::Gamepad::GamepadEvent evt, gameplay::Gamepad * gamepad) override;
        virtual void update(float elapsedTime) override;
        virtual void render(float elapsedTime) override;

    private:
        void renderLoadingMessage(void * message);
        void loadCachedAssets();
        void unloadCachedAssets();
        void broadcastKeyEvent(gameplay::Keyboard::KeyEvent evt, int key);
        void renderSplashScreen(void * = nullptr);
        void setSplashScreenFade(float duration, bool isFadingIn);

#ifndef _FINAL
        gameplay::Font * _debugFont;
#endif
        gameplay::AIMessage * _pinchMessage;
        gameplay::AIMessage * _keyMessage;
        gameplay::AIMessage * _touchMessage;
        gameplay::AIMessage * _mouseMessage;
        gameplay::AIMessage * _gamepadMessage;
        gameplay::SpriteBatch * _splashForegroundSpriteBatch;
        gameplay::SpriteBatch * _splashBackgroundSpriteBatch;
        std::vector<gameplay::Texture *> _cachedTextures;
        std::vector<SpriteSheet *> _cachedSpriteSheets;
        std::queue<std::pair<float,float>> _splashScreenFadeRequests;
        float _splashScreenAlpha;
        float _splashScreenFadeDuration;
        float _splashScreenFadeDirection;
        bool _splashScreenFadeActive;
#ifndef WIN32
        int _previousReleasedKey;
        int _framesSinceKeyReleaseEvent;
#endif
    };
}

#endif
