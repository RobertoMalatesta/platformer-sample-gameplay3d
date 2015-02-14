#ifndef GAME_SCREEN_RENDERER
#define GAME_SCREEN_RENDERER

#include <queue>
#include "Rectangle.h"
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
        bool render();
        void renderImmediate();
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
        void renderSpinner();
        void updateRequests(float dt = 0.0f);

        gameplay::Rectangle _logoSrc;
        gameplay::Rectangle _spinnerSrc;
        gameplay::Rectangle _spinnerDst;
        gameplay::Rectangle _bannersDst;
        gameplay::Rectangle _bannersSrc;
        gameplay::SpriteBatch * _texuturesSpriteBatch;
        gameplay::SpriteBatch * _fillSpriteBatch;
        std::queue<Request> _requests;
        float _alpha;
        float _previousAlpha;
        float _fadeDuration;
        float _isFadingIn;
        float _fadeTimer;
        float _previousSpinnerTime;
        float _spinnerRotation;
        bool _fadeActive;
        bool _wasUpdatedThisFrame;
        Request::Type  _requestType;
        gameplay::Vector4 _backgroundColor;
    };
}

#endif
