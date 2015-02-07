#include "ScreenRenderer.h"

#include "Common.h"
#include "Font.h"
#include "Game.h"
#include "ResourceManager.h"
#include "SpriteSheet.h"

namespace  game
{
    static const gameplay::Vector4 LOADING_BG_COLOR(gameplay::Vector4::fromColor(0xF5F5F5FF));

    ScreenRenderer::ScreenRenderer()
        : _alpha(1.0f)
        , _fadeDuration(0.0f)
        , _fadeActive(false)
        , _texuturesSpriteBatch(nullptr)
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

        SpriteSheet * textureSpritesheet = ResourceManager::getInstance().getSpriteSheet("res/spritesheets/screen.ss");
        _spinnerSrc = textureSpritesheet->getSprite("spinner")->_src;
        _spinnerDst = _spinnerSrc;
        _spinnerDst.x = (gameplay::Game::getInstance()->getWidth() / 2) - (_spinnerDst.width / 2);
        _spinnerDst.y = (gameplay::Game::getInstance()->getHeight() / 2) - (_spinnerDst.height / 2);

        _bannersSrc = textureSpritesheet->getSprite("banners")->_src;
        _bannersDst = _bannersSrc;
        _bannersDst.x = (gameplay::Game::getInstance()->getWidth() / 2) - (_bannersDst.width / 2);
        _bannersDst.y = (gameplay::Game::getInstance()->getHeight() / 2) + _spinnerDst.height / 2;

        _texuturesSpriteBatch = gameplay::SpriteBatch::create(textureSpritesheet->getTexture());
        SAFE_RELEASE(textureSpritesheet);

        queueFadeToLoadingScreen(0.0f);
    }

    void ScreenRenderer::finalize()
    {
        SAFE_DELETE(_fillSpriteBatch);
        SAFE_DELETE(_texuturesSpriteBatch);
    }

    void ScreenRenderer::update(float elapsedTime)
    {
        updateRequests(elapsedTime / 1000.0f);
        _wasUpdatedThisFrame = false;
    }

    void ScreenRenderer::updateRequests(float dt)
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

            if(_requestType == Request::Type::LOADING_SPINNER)
            {
                _texuturesSpriteBatch->start();

                static float previousSpinnerTime = gameplay::Game::getAbsoluteTime();
                static float rotation = 0.0f;
                rotation += MATH_CLAMP((gameplay::Game::getAbsoluteTime() - previousSpinnerTime) / 500.0f, 0, 1.0f / 15.0f);

                _texuturesSpriteBatch->draw(gameplay::Vector3(_spinnerDst.x, _spinnerDst.y + _spinnerDst.height, 0),
                                            _spinnerSrc, gameplay::Vector2(_spinnerDst.width, -_spinnerDst.height),
                                            gameplay::Vector4(1, 1, 1, _alpha),
                                            gameplay::Vector2(0.5f, 0.5f),
                                            rotation);
                previousSpinnerTime = gameplay::Game::getAbsoluteTime();

                _texuturesSpriteBatch->draw(_bannersDst, _bannersSrc, gameplay::Vector4(1, 1, 1, _alpha));
                _texuturesSpriteBatch->finish();
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
        updateRequests();
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
        request._backgroundColor = LOADING_BG_COLOR;
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
