#ifndef GAME_H
#define GAME_H

#include "Game.h"
#include "GameObjectMessage.h"

namespace gameplay
{
    class Font;
    class SpriteBatch;
    class Texture;
}

namespace game
{
    class PropertiesRef;
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
        virtual void update(float elapsedTime) override;
        virtual void render(float elapsedTime) override;

    private:
        void renderLoadingMessage(void * message);
        void loadCachedAssets();
        void unloadCachedAssets();
        void broadcastKeyEvent(gameplay::Keyboard::KeyEvent evt, int key);
        void renderSplashScreen(void * = nullptr);
        void setSplashScreenFade(float duration, bool isFadingIn, bool showLogo);
        void updateSplashScreenFade(float dt = 0.0f);

#ifndef _FINAL
        gameplay::Font * _debugFont;
#endif
        gameobjects::Message * _pinchMessage;
        gameobjects::Message * _keyMessage;
        gameobjects::Message * _touchMessage;
        gameobjects::Message * _mouseMessage;
        gameplay::SpriteBatch * _splashForegroundSpriteBatch;
        gameplay::SpriteBatch * _splashBackgroundSpriteBatch;
        std::vector<gameplay::Texture *> _cachedTextures;
        std::vector<PropertiesRef *> _cachedProperties;
        std::vector<SpriteSheet *> _cachedSpriteSheets;
        std::queue<std::tuple<float,bool,bool>> _splashScreenFadeRequests;
        float _splashScreenAlpha;
        float _splashScreenFadeDuration;
        float _splashScreenIsFadingIn;
        float _splashScreenFadeTimer;
        bool _splashScreenFadeActive;
        bool _splashScreenShowsLogo;
        bool _splashScreenUpdatedThisFrame;
#ifndef WIN32
        int _previousReleasedKey;
        int _framesSinceKeyReleaseEvent;
#endif
    };
}

#endif
