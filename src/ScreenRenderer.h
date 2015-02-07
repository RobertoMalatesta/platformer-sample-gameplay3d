#ifndef GAME_SCREEN_RENDERER
#define GAME_SCREEN_RENDERER

#include <queue>
#include "Vector4.h"

namespace gameplay
{
    class SpriteBatch;
}

namespace game
{
    /** @script{ignore} */
    class ScreenRenderer
    {
    public:
        static ScreenRenderer & getInstance();
        void initialize();
        void finalize();
        void update(float elapsedTime);
        void render();
        bool isVisible() const;

        void queueFadeToBlack(float duration = 1.0f);
        void queueFadeToLoadingScreen(float duration = 1.0f);
        void queueFadeOut(float duration = 1.0f);
    private:
        struct Request
        {
            enum Type
            {
                COLOR_FILL,
                LOADING_SPINNER
            };

            bool _isFadingIn;
            float _duration;
            Type _type;
            gameplay::Vector4 _backgroundColor;
        };

        explicit ScreenRenderer();
        ~ScreenRenderer();
        ScreenRenderer(ScreenRenderer const &);

        void queue(Request const & request);
        void renderImmediate();
        void updateSplashScreenFade(float dt = 0.0f);

        gameplay::SpriteBatch * _loadingForegroundSpriteBatch;
        gameplay::SpriteBatch * _fillSpriteBatch;
        std::queue<Request> _requests;
        float _alpha;
        float _fadeDuration;
        float _isFadingIn;
        float _fadeTimer;
        bool _fadeActive;
        bool _wasUpdatedThisFrame;
        Request::Type  _requestType;
        gameplay::Vector4 _backgroundColor;
    };
}

#endif
