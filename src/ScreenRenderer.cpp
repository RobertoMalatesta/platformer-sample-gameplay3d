#include "ScreenRenderer.h"

#include "Common.h"
#include "Font.h"
#include "Game.h"
#include "ResourceManager.h"

namespace  game
{
    ScreenRenderer::ScreenRenderer()
        : _alpha(1.0f)
        , _fadeDuration(0.0f)
        , _fadeActive(false)
        , _loadingForegroundSpriteBatch(nullptr)
        , _fillSpriteBatch(nullptr)
        , _isFadingIn(true)
        , _requestType(Request::Type::COLOR_FILL)
        , _wasUpdatedThisFrame(false)
        , _fadeTimer(0.0f)
        , _backgroundColor(gameplay::Vector4::one())
    {
    }

    ScreenRenderer::~ScreenRenderer()
    {
    }

    ScreenRenderer::ScreenRenderer(ScreenRenderer const &)
    {
    }

    ScreenRenderer & ScreenRenderer::getInstance()
    {
        static ScreenRenderer instance;
        return instance;
    }

    void ScreenRenderer::initialize()
    {
        _fillSpriteBatch = ResourceManager::getInstance().createSinglePixelSpritebatch();
        _loadingForegroundSpriteBatch = gameplay::SpriteBatch::create("@res/textures/splash");
        queueFadeToLoadingScreen(0.0f);
    }

    void ScreenRenderer::finalize()
    {
        SAFE_DELETE(_fillSpriteBatch);
        SAFE_DELETE(_loadingForegroundSpriteBatch);
    }

    void ScreenRenderer::update(float elapsedTime)
    {
        updateSplashScreenFade(elapsedTime / 1000.0f);
        _wasUpdatedThisFrame = false;
    }

    void ScreenRenderer::updateSplashScreenFade(float dt)
    {
        if(_wasUpdatedThisFrame)
        {
            return;
        }

        if(!_fadeActive && !_requests.empty())
        {
            Request & request = _requests.front();
            _requests.pop();
            _fadeDuration = request._duration;
            _isFadingIn = request._isFadingIn;
            _requestType = request._type;
            _backgroundColor = request._backgroundColor;
            _fadeTimer = _fadeDuration;
            _alpha = _isFadingIn ? 0.0f : 1.0f;
            _fadeActive = true;
        }

        if(_fadeActive)
        {
            static float const minPlayableDt = 1.0f / 15.0f;

            if(_fadeTimer > 0.0f)
            {
                if (dt <= minPlayableDt)
                {
                    _fadeTimer = MATH_CLAMP(_fadeTimer - dt, 0.0f, std::numeric_limits<float>::max());
                    float const startAlpha = _isFadingIn ? 0.0f : 1.0f;
                    float const alphaDt = (1.0f / _fadeDuration) *
                            (_fadeDuration - _fadeTimer) *
                            (_isFadingIn ? 1.0f : -1.0f);
                    _alpha = startAlpha + alphaDt;
                }
            }
            else
            {
                _alpha = _isFadingIn ? 1.0f : 0.0f;
                _fadeActive = false;
                _fadeDuration = 0.0f;
                _fadeTimer = 0.0f;
            }
        }

        _wasUpdatedThisFrame = true;
    }


    void ScreenRenderer::render()
    {
        if(_alpha > 0.0f)
        {
            gameplay::Font * debugFont = ResourceManager::getInstance().getDebugFront();
            gameplay::Vector4 bgColor = _backgroundColor;
            bgColor.w = _alpha;

            _fillSpriteBatch->start();
            _fillSpriteBatch->draw(gameplay::Rectangle(0, 0, gameplay::Game::getInstance()->getWidth(), gameplay::Game::getInstance()->getHeight()),
                                               gameplay::Rectangle(0, 0, 1, 1),
                                               bgColor);
            _fillSpriteBatch->finish();

            if (_requestType == Request::Type::LOADING_SPINNER)
            {
                gameplay::Rectangle bounds(_loadingForegroundSpriteBatch->getSampler()->getTexture()->getWidth(),
                    _loadingForegroundSpriteBatch->getSampler()->getTexture()->getHeight());
                gameplay::Vector2 drawPos;
                drawPos.x = (gameplay::Game::getInstance()->getWidth() / 2) - (bounds.width / 2);
                drawPos.y = (gameplay::Game::getInstance()->getHeight() / 2) - (bounds.height / 2);

                _loadingForegroundSpriteBatch->start();
                _loadingForegroundSpriteBatch->draw(gameplay::Rectangle(drawPos.x, drawPos.y, bounds.width, bounds.height),
                    gameplay::Rectangle(0, 0, bounds.width, bounds.height),
                    gameplay::Vector4(1, 1, 1, _alpha));
                _loadingForegroundSpriteBatch->finish();
            }

#if !defined(_FINAL) && !defined(__ANDROID__)
            if(_alpha == 1.0f && gameplay::Game::getInstance()->getConfig()->getBool("debug_enable_tools"))
            {
                debugFont->start();
                debugFont->drawText("Running tools...", 5, 5, gameplay::Vector4(1, 0, 0, 1));
                debugFont->finish();
            }
#endif
        }
    }

    void ScreenRenderer::queue(Request const & request)
    {
        _requests.push(request);
        updateSplashScreenFade();
        renderImmediate();
    }

    void ScreenRenderer::queueFadeToBlack(float duration)
    {
        Request request;
        request._duration = duration;
        request._type = Request::Type::COLOR_FILL;
        request._isFadingIn = true;
        request._backgroundColor = gameplay::Vector4::fromColor(0x000000FF);
        queue(request);
    }

    void ScreenRenderer::queueFadeToLoadingScreen(float duration)
    {
        Request request;
        request._duration = duration;
        request._type = Request::Type::LOADING_SPINNER;
        request._isFadingIn = true;
        request._backgroundColor = gameplay::Vector4::fromColor(0xF5F5F5FF);
        queue(request);
    }

    void ScreenRenderer::queueFadeOut(float duration)
    {
        Request request;
        request._duration = duration;
        request._type = _requestType;
        request._isFadingIn = false;
        request._backgroundColor = _backgroundColor;
        queue(request);
    }

    void ScreenRenderer::renderImmediate()
    {
        if(_alpha > 0)
        {
            render();
            gameplay::Platform::swapBuffers();
        }
    }

    bool ScreenRenderer::isVisible() const
    {
        return _alpha > 0.0f;
    }
}
