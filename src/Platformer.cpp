#include "Common.h"
#include "Platformer.h"

#include "CameraControlComponent.h"
#include "CollisionHandlerComponent.h"
#include "CollisionObjectComponent.h"
#include "EnemyComponent.h"
#include "GameObjectController.h"
#include "LevelComponent.h"
#include "LevelRendererComponent.h"
#include "Messages.h"
#include "PlayerAudioComponent.h"
#include "PlayerComponent.h"
#include "PlayerHandOfGodComponent.h"
#include "PlayerInputComponent.h"
#include "PlatformerEventForwarderComponent.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "SpriteSheet.h"
#include "SpriteAnimationComponent.h"

game::Platformer platformer;

namespace game
{
    Platformer::Platformer()
        : _keyMessage(nullptr)
        , _pinchMessage(nullptr)
        , _touchMessage(nullptr)
        , _mouseMessage(nullptr)
        , _splashScreenAlpha(1.0f)
        , _splashScreenFadeDuration(0.0f)
        , _splashScreenFadeActive(false)
        , _splashForegroundSpriteBatch(nullptr)
        , _splashBackgroundSpriteBatch(nullptr)
        , _splashScreenIsFadingIn(true)
        , _splashScreenShowsLogo(true)
        , _splashScreenUpdatedThisFrame(false)
        , _splashScreenFadeTimer(0.0f)
#ifndef WIN32
        , _previousReleasedKey(gameplay::Keyboard::Key::KEY_NONE)
        , _framesSinceKeyReleaseEvent(0)
#endif
#ifndef _FINAL
        , _debugFont(nullptr)
#endif
    {
    }

    Platformer::~Platformer()
    {
    }

    class PlatformerGameObjectCallbackHandler : public gameobjects::GameObjectCallbackHandler
    {
    public:
        virtual void OnPreCreateProperties(const char * url) override
        {
            GAME_LOG("Loading properties '%s'", url);
        }
    };

    void Platformer::initialize()
    {
        gameplay::Logger::set(gameplay::Logger::Level::LEVEL_INFO, loggingCallback);
        gameplay::Logger::set(gameplay::Logger::Level::LEVEL_WARN, loggingCallback);
        gameplay::Logger::set(gameplay::Logger::Level::LEVEL_ERROR, loggingCallback);

#ifndef _FINAL
    #ifdef WIN32
        getConfig()->setString("debug_os", "windows");
    #else
        getConfig()->setString("debug_os", "linux");
    #endif

        _debugFont = gameplay::Font::create(getConfig()->getString("debug_font"));
#endif
        // Display a splash screen during loading, consider displaying a loading progress indicator once initial load time lasts more
        // than a few seconds on weakest target hardware
        _splashBackgroundSpriteBatch = ResourceManager::createSinglePixelSpritebatch();
        _splashForegroundSpriteBatch = gameplay::SpriteBatch::create("@res/textures/splash");
        renderOnce(this, &Platformer::renderSplashScreen, nullptr);

#if !_FINAL && !NO_LUA_BINDINGS
        if(getConfig()->getBool("debug_enable_tools"))
        {
            getScriptController()->loadScript("res/lua/run_tools.lua");

            if(getConfig()->getBool("debug_run_tools_only"))
            {
                exit();
                return;
            }
        }
#endif
        setMultiTouch(true);

        if (isGestureSupported(gameplay::Gesture::GESTURE_PINCH))
        {
            registerGesture(gameplay::Gesture::GESTURE_PINCH);
        }

        if(gameplay::Properties * windowSettings = getConfig()->getNamespace("window", true))
        {
            char const * vsyncOption = "vsync";
            if(windowSettings->exists(vsyncOption))
            {
                setVsync(windowSettings->getBool(vsyncOption));
            }
        }

        ResourceManager::getInstance().initialize();

        // Register the component types so the GameObject system will know how to serialize them from the .go files
        gameobjects::GameObjectController::getInstance().registerComponent<CameraControlComponent>("camera_control");
        gameobjects::GameObjectController::getInstance().registerComponent<CollisionObjectComponent>("collision_object");
        gameobjects::GameObjectController::getInstance().registerComponent<EnemyComponent>("enemy");
        gameobjects::GameObjectController::getInstance().registerComponent<LevelComponent>("level");
        gameobjects::GameObjectController::getInstance().registerComponent<LevelRendererComponent>("level_renderer");
        gameobjects::GameObjectController::getInstance().registerComponent<PlayerAudioComponent>("player_audio");
        gameobjects::GameObjectController::getInstance().registerComponent<PlayerComponent>("player");
        gameobjects::GameObjectController::getInstance().registerComponent<PlayerHandOfGodComponent>("player_hand_of_god");
        gameobjects::GameObjectController::getInstance().registerComponent<PlayerInputComponent>("player_input");
        gameobjects::GameObjectController::getInstance().registerComponent<PlatformerEventForwarderComponent>("platformer_event_forwarder");
        gameobjects::GameObjectController::getInstance().registerComponent<SpriteAnimationComponent>("sprite_animation");
        gameobjects::GameObjectController::getInstance().registerComponent<CollisionHandlerComponent>("collision_handler");
        PlatformerGameObjectCallbackHandler * callbackHandler = new PlatformerGameObjectCallbackHandler();
        gameobjects::GameObjectController::getInstance().registerCallbackHandler(callbackHandler);
        gameobjects::GameObjectController::getInstance().initialize();

        _pinchMessage = PinchMessage::create();
        _keyMessage = KeyMessage::create();
        _touchMessage = TouchMessage::create();
        _mouseMessage = MouseMessage::create();

        getAudioListener()->setCamera(nullptr);
        gameobjects::GameObject * rootGameObject = gameobjects::GameObjectController::getInstance().createGameObject("root");
        gameobjects::GameObjectController::getInstance().createGameObject("level_0", rootGameObject);
    }

    void Platformer::finalize()
    {
        // Only perform resource cleanup for the purposes of detecting memory leaks, in final builds we want
        // to shutdown as fast as possible, the platform will free up all resources used by this process
#ifdef GP_USE_MEM_LEAK_DETECTION
        gameobjects::GameObjectController::getInstance().finalize();
        GAMEOBJECTS_DELETE_MESSAGE(_pinchMessage);
        GAMEOBJECTS_DELETE_MESSAGE(_keyMessage);
        GAMEOBJECTS_DELETE_MESSAGE(_touchMessage);
        GAMEOBJECTS_DELETE_MESSAGE(_mouseMessage);
        SAFE_RELEASE(_debugFont);
        SAFE_DELETE(_splashBackgroundSpriteBatch);
        SAFE_DELETE(_splashForegroundSpriteBatch);
        ResourceManager::getInstance().finalize();

        // Removing log handlers once everything else has been torn down
        clearLogHistory();
        void(*nullFuncPtr) (gameplay::Logger::Level, const char*) = nullptr;
        gameplay::Logger::set(gameplay::Logger::Level::LEVEL_INFO, nullFuncPtr);
        gameplay::Logger::set(gameplay::Logger::Level::LEVEL_WARN, nullFuncPtr);
        gameplay::Logger::set(gameplay::Logger::Level::LEVEL_ERROR, nullFuncPtr);
#endif
    }

    void Platformer::broadcastKeyEvent(gameplay::Keyboard::KeyEvent evt, int key)
    {
        KeyMessage::setMessage(_keyMessage, evt, key);
        gameobjects::GameObjectController::getInstance().broadcastMessage(_keyMessage);
    }

    void Platformer::gesturePinchEvent(int x, int y, float scale)
    {
        if(_pinchMessage && _splashScreenAlpha == 0.0f)
        {
            PinchMessage::setMessage(_pinchMessage, x, y, scale);
            gameobjects::GameObjectController::getInstance().broadcastMessage(_pinchMessage);
        }
    }

    void Platformer::keyEvent(gameplay::Keyboard::KeyEvent evt, int key)
    {
        if (_keyMessage && _splashScreenAlpha == 0.0f)
        {
#ifndef WIN32
            if(evt != gameplay::Keyboard::KeyEvent::KEY_RELEASE)
            {
                if(key != _previousReleasedKey || _framesSinceKeyReleaseEvent > 1)
                {
                    broadcastKeyEvent(evt, key);
                }
                else
                {
                    _framesSinceKeyReleaseEvent = 0;
                    _previousReleasedKey = gameplay::Keyboard::Key::KEY_NONE;
                }
            }
            else
            {
                _previousReleasedKey = key;
            }
#else
            broadcastKeyEvent(evt, key);
#endif
        }

        if (evt == gameplay::Keyboard::KEY_PRESS && key == gameplay::Keyboard::KEY_ESCAPE)
        {
            exit();
        }
    }

    void Platformer::touchEvent(gameplay::Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
    {
        if (_touchMessage && _splashScreenAlpha == 0.0f)
        {
            TouchMessage::setMessage(_touchMessage, evt, x, y, contactIndex);
            gameobjects::GameObjectController::getInstance().broadcastMessage(_touchMessage);
        }
    }

    bool Platformer::mouseEvent(gameplay::Mouse::MouseEvent evt, int x, int y, int wheelDelta)
    {
        if (_mouseMessage && _splashScreenAlpha == 0.0f)
        {
            MouseMessage::setMessage(_mouseMessage, evt, x, y, wheelDelta);
            gameobjects::GameObjectController::getInstance().broadcastMessage(_mouseMessage);
        }
        
        return false;
    }

    void Platformer::updateSplashScreenFade(float dt)
    {
        if(_splashScreenUpdatedThisFrame)
        {
            return;
        }

        if(!_splashScreenFadeActive && !_splashScreenFadeRequests.empty())
        {
            auto & request = _splashScreenFadeRequests.front();
            _splashScreenFadeRequests.pop();
            _splashScreenFadeDuration = std::get<0>(request);
            _splashScreenIsFadingIn = std::get<1>(request);
            _splashScreenShowsLogo = std::get<2>(request);
            _splashScreenFadeTimer = _splashScreenFadeDuration;
            _splashScreenFadeActive = true;
        }

        if(_splashScreenFadeActive)
        {
            static float const minPlayableDt = 1.0f / 15.0f;

            if(_splashScreenFadeTimer > 0.0f)
            {
                if (dt <= minPlayableDt)
                {
                    _splashScreenFadeTimer = MATH_CLAMP(_splashScreenFadeTimer - dt, 0.0f, std::numeric_limits<float>::max());
                    float const startAlpha = _splashScreenIsFadingIn ? 0.0f : 1.0f;
                    float const alphaDt = (1.0f / _splashScreenFadeDuration) *
                            (_splashScreenFadeDuration - _splashScreenFadeTimer) *
                            (_splashScreenIsFadingIn ? 1.0f : -1.0f);
                    _splashScreenAlpha = startAlpha + alphaDt;
                }
            }
            else
            {
                _splashScreenAlpha = _splashScreenIsFadingIn ? 1.0f : 0.0f;
                _splashScreenFadeActive = false;
                _splashScreenFadeDuration = 0.0f;
                _splashScreenFadeTimer = 0.0f;
            }
        }

        _splashScreenUpdatedThisFrame = true;
    }

    void Platformer::update(float elapsedTime)
    {
        // Work-around for receiving false key release events due to auto repeat in x11
#ifndef WIN32
        if(_previousReleasedKey != gameplay::Keyboard::Key::KEY_NONE)
        {
            ++_framesSinceKeyReleaseEvent;

            if(_framesSinceKeyReleaseEvent > 1)
            {
                broadcastKeyEvent(gameplay::Keyboard::KeyEvent::KEY_RELEASE, _previousReleasedKey);
                _framesSinceKeyReleaseEvent = 0;
                _previousReleasedKey = gameplay::Keyboard::Key::KEY_NONE;
            }
        }
#endif

        gameobjects::GameObjectController::getInstance().update(elapsedTime);
        updateSplashScreenFade(elapsedTime / 1000.0f);
        _splashScreenUpdatedThisFrame = false;
    }

    void Platformer::render(float elapsedTime)
    {
        clear(gameplay::Game::CLEAR_COLOR_DEPTH, gameplay::Vector4::zero(), 1.0f, 0);
        gameobjects::GameObjectController::getInstance().render(elapsedTime);
        renderSplashScreen();

        if (_splashScreenAlpha <= 0.0f)
        {
            if (gameplay::Gamepad * gamepad = getGamepad(0))
            {
                if (gameplay::Form * gamepadForm = gamepad->getForm())
                {
                    gamepadForm->draw();
                }
            }
        }

#ifndef _FINAL
        gameobjects::GameObjectController::getInstance().renderDebug(elapsedTime, _debugFont);

        if (gameplay::Game::getInstance()->getConfig()->getBool("debug_draw_physics"))
        {
            gameplay::Game::getInstance()->getPhysicsController()->drawDebug(
                        gameobjects::GameObjectController::getInstance().getScene()->getActiveCamera()->getViewProjectionMatrix());
        }

        std::array<char, CHAR_MAX> buffer;
        static const int paddingX = 5;
        int const paddingY = _debugFont->getSize();
        int y = paddingY / 2;
        _debugFont->start();

        if (gameplay::Game::getInstance()->getConfig()->getBool("debug_show_fps"))
        {
            sprintf(&buffer[0], "%d FPS", getFrameRate());
            _debugFont->drawText(&buffer[0], paddingX, y, gameplay::Vector4(1, 0, 0, 1));
            y += paddingY;
        }

        if (gameplay::Game::getInstance()->getConfig()->getBool("debug_show_resolution"))
        {
            sprintf(&buffer[0], "%dx%d", getWidth(), getHeight());
            _debugFont->drawText(&buffer[0], paddingX, y, gameplay::Vector4(1, 0, 0, 1));
            y += paddingY;
        }

        _debugFont->finish();
#endif
    }

    void Platformer::renderSplashScreen(void *)
    {
        if(_splashScreenAlpha > 0.0f)
        {
            gameplay::Vector4 bgColor = gameplay::Vector4::fromColor(_splashScreenShowsLogo ? 0xF5F5F5FF : 0x0000FF);
            bgColor.w = _splashScreenAlpha;

            _splashBackgroundSpriteBatch->start();
            _splashBackgroundSpriteBatch->draw(gameplay::Rectangle(0, 0, getWidth(), getHeight()),
                                               gameplay::Rectangle(0, 0, 1, 1),
                                               bgColor);
            _splashBackgroundSpriteBatch->finish();

            if (_splashScreenShowsLogo)
            {
                gameplay::Rectangle bounds(_splashForegroundSpriteBatch->getSampler()->getTexture()->getWidth(),
                    _splashForegroundSpriteBatch->getSampler()->getTexture()->getHeight());
                gameplay::Vector2 drawPos;
                drawPos.x = (getWidth() / 2) - (bounds.width / 2);
                drawPos.y = (getHeight() / 2) - (bounds.height / 2);

                _splashForegroundSpriteBatch->start();
                _splashForegroundSpriteBatch->draw(gameplay::Rectangle(drawPos.x, drawPos.y, bounds.width, bounds.height),
                    gameplay::Rectangle(0, 0, bounds.width, bounds.height),
                    gameplay::Vector4(1, 1, 1, _splashScreenAlpha));
                _splashForegroundSpriteBatch->finish();
            }

#if !defined(_FINAL) && !defined(__ANDROID__)
            if(_splashScreenAlpha == 1.0f && getConfig()->getBool("debug_enable_tools"))
            {
                _debugFont->start();
                _debugFont->drawText("Running tools...", 5, 5, gameplay::Vector4(1, 0, 0, 1));
                _debugFont->finish();
            }
#endif
        }
    }

    void Platformer::setSplashScreenFade(float duration, bool isFadingIn, bool showLogo)
    {
        _splashScreenFadeRequests.push(std::make_tuple(duration, isFadingIn, showLogo));
        updateSplashScreenFade();
        renderOnce(this, &Platformer::renderSplashScreen, nullptr);
    }
}
