#include "Platformer.h"

#include "CameraControlComponent.h"
#include "CollisionObjectComponent.h"
#include "Common.h"
#include "EnemyComponent.h"
#include "GameObjectController.h"
#include "LevelComponent.h"
#include "MessagesInput.h"
#include "PlayerAudioComponent.h"
#include "PlayerComponent.h"
#include "PlayerHandOfGodComponent.h"
#include "PlayerInputComponent.h"
#include "PlatformerEventForwarderComponent.h"
#include "SpriteAnimationComponent.h"
#include "CollisionHandlerComponent.h"
#include "LevelRendererComponent.h"

platformer::Platformer game;

namespace platformer
{
    Platformer::Platformer()
        : _keyMessage(nullptr)
        , _pinchMessage(nullptr)
        , _touchMessage(nullptr)
        , _mouseMessage(nullptr)
        , _gamepadMessage(nullptr)
        , _splashScreenAlpha(1.0f)
        , _splashScreenFadeDuration(0.0f)
        , _splashScreenFadeActive(false)
        , _splashForegroundSpriteBatch(nullptr)
        , _splashBackgroundSpriteBatch(nullptr)
        , _splashScreenFadeDirection(0.0f)
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
            PLATFORMER_LOG("Loading properties '%s'", url);
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
    #elif defined(__ANDROID__)
        getConfig()->setString("debug_os", "android");
        getConfig()->setString("debug_enable_tools", "false");
        getConfig()->setString("debug_run_tools_only", "false");
    #else
        getConfig()->setString("debug_os", "linux");
    #endif

        _debugFont = gameplay::Font::create(getConfig()->getString("debug_font"));
#endif

        // Display a splash screen during loading, consider displaying a loading progress indicator once initial load time lasts more
        // than a few seconds on weakest target hardware
        _splashBackgroundSpriteBatch = createSinglePixelSpritebatch();
        _splashForegroundSpriteBatch = gameplay::SpriteBatch::create("@res/textures/splash");
        renderOnce(this, &Platformer::renderSplashScreen, nullptr);

        std::vector<std::string> fileList;

        std::string const textureDirectory = gameplay::FileSystem::resolvePath("@res/textures");
        gameplay::FileSystem::listFiles(textureDirectory.c_str(), fileList);

        for (std::string & fileName : fileList)
        {
            std::string const textureFile = textureDirectory + "/" + fileName;
            gameplay::Texture * texture = gameplay::Texture::create(textureFile.c_str());
            texture->addRef();
            _cachedTextures.push_back(texture);
        }

        fileList.clear();
        std::string const spriteSheetDirectory = "res/spritesheets";
        gameplay::FileSystem::listFiles(gameplay::FileSystem::resolvePath(spriteSheetDirectory.c_str()), fileList);

        for (std::string & fileName : fileList)
        {
            std::string const spriteSheetFile = spriteSheetDirectory + "/" + fileName;
            SpriteSheet * spriteSheet = SpriteSheet::create(spriteSheetFile.c_str());
            spriteSheet->addRef();
            _cachedSpriteSheets.push_back(spriteSheet);
        }

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
        _gamepadMessage = GamepadMessage::create();

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
    }

    void Platformer::finalize()
    {
        gameobjects::GameObjectController::getInstance().finalize();

        PLATFORMER_SAFE_DELETE_AI_MESSAGE(_pinchMessage);
        PLATFORMER_SAFE_DELETE_AI_MESSAGE(_keyMessage);
        PLATFORMER_SAFE_DELETE_AI_MESSAGE(_touchMessage);
        PLATFORMER_SAFE_DELETE_AI_MESSAGE(_mouseMessage);
        PLATFORMER_SAFE_DELETE_AI_MESSAGE(_gamepadMessage);
        
#ifndef _FINAL
        SAFE_RELEASE(_debugFont);
#endif
        SAFE_DELETE(_splashBackgroundSpriteBatch);
        SAFE_DELETE(_splashForegroundSpriteBatch);

        int const unusedCachedAssetRefCount = 2;

        for (SpriteSheet * spriteSheet : _cachedSpriteSheets)
        {
            PLATFORMER_ASSERT(spriteSheet->getRefCount() == unusedCachedAssetRefCount, "Unreleased spritesheet found");
            PLATFORMER_FORCE_RELEASE(spriteSheet);
        }

        _cachedSpriteSheets.clear();

        for (gameplay::Texture * texture : _cachedTextures)
        {
            PLATFORMER_ASSERT(texture->getRefCount() == unusedCachedAssetRefCount, "Unreleased texture found");
            PLATFORMER_FORCE_RELEASE(texture);
        }

        _cachedTextures.clear();

        // Removing log handlers once everything else has been torn down
#ifndef _FINAL
        clearLogHistory();
#endif
        void(*nullFuncPtr) (gameplay::Logger::Level, const char*) = nullptr;
        gameplay::Logger::set(gameplay::Logger::Level::LEVEL_INFO, nullFuncPtr);
        gameplay::Logger::set(gameplay::Logger::Level::LEVEL_WARN, nullFuncPtr);
        gameplay::Logger::set(gameplay::Logger::Level::LEVEL_ERROR, nullFuncPtr);
    }

    void Platformer::broadcastKeyEvent(gameplay::Keyboard::KeyEvent evt, int key)
    {
        KeyMessage::setMessage(_keyMessage, evt, key);
        gameobjects::GameObjectController::getInstance().broadcastGameObjectMessage(_keyMessage);
    }

    void Platformer::gesturePinchEvent(int x, int y, float scale)
    {
        if(_pinchMessage)
        {
            PinchMessage::setMessage(_pinchMessage, x, y, scale);
            gameobjects::GameObjectController::getInstance().broadcastGameObjectMessage(_pinchMessage);
        }
    }

    void Platformer::keyEvent(gameplay::Keyboard::KeyEvent evt, int key)
    {
        if (_keyMessage)
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
        if (_touchMessage)
        {
            TouchMessage::setMessage(_touchMessage, evt, x, y, contactIndex);
            gameobjects::GameObjectController::getInstance().broadcastGameObjectMessage(_touchMessage);
        }
    }

    bool Platformer::mouseEvent(gameplay::Mouse::MouseEvent evt, int x, int y, int wheelDelta)
    {
        if (_mouseMessage)
        {
            MouseMessage::setMessage(_mouseMessage, evt, x, y, wheelDelta);
            gameobjects::GameObjectController::getInstance().broadcastGameObjectMessage(_mouseMessage);
        }
        
        return false;
    }

    void Platformer::gamepadEvent(gameplay::Gamepad::GamepadEvent evt, gameplay::Gamepad * gamepad)
    {
        if(_gamepadMessage)
        {
            for(int i = 0; i < PLATFORMER_MAX_GAMEPADS; ++i)
            {
                if(gamepad == getGamepad(i))
                {
                    GamepadMessage::setMessage(_gamepadMessage, evt, i);
                    gameobjects::GameObjectController::getInstance().broadcastGameObjectMessage(_gamepadMessage);
                    break;
                }
            }
        }
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

        gameobjects::GameObjectController::getInstance().independentUpdate(abs(elapsedTime));

        if (getState() == gameplay::Game::State::RUNNING)
        {
            gameobjects::GameObjectController::getInstance().update(elapsedTime);
        }

        if(!_splashScreenFadeActive && !_splashScreenFadeRequests.empty())
        {
            auto & request = _splashScreenFadeRequests.front();
            _splashScreenFadeRequests.pop();
            _splashScreenFadeDuration = request.first;
            _splashScreenFadeDirection = request.second;
            _splashScreenFadeActive = true;
        }

        if(_splashScreenFadeActive)
        {
            static float const minAlpha = 0.0f;
            static float const maxAlpha = 1.0f;

            float splashScreenAlphaDelta = 0.0f;

            if(_splashScreenFadeDuration != 0)
            {
                static int const minFpsToFade = 10;
                if (getFrameRate() > minFpsToFade)
                {
                    float const durationInFrames = static_cast<float>(getFrameRate()) * _splashScreenFadeDuration;
                    splashScreenAlphaDelta = (1.0f / durationInFrames) * _splashScreenFadeDirection;
                }
            }
            else
            {
                splashScreenAlphaDelta = _splashScreenFadeDirection;
            }

            _splashScreenAlpha = MATH_CLAMP(_splashScreenAlpha + splashScreenAlphaDelta, minAlpha, maxAlpha);

            if((_splashScreenAlpha == minAlpha && _splashScreenFadeDirection < 0) || (_splashScreenAlpha == maxAlpha && _splashScreenFadeDirection > 0))
            {
                _splashScreenFadeActive = false;
                _splashScreenFadeDuration = 0.0f;
            }
        }
    }

    void Platformer::render(float elapsedTime)
    {
        clear(gameplay::Game::CLEAR_COLOR_DEPTH, gameplay::Vector4::zero(), 1.0f, 0);
        gameobjects::GameObjectController::getInstance().render(elapsedTime);
        renderSplashScreen();

#ifndef _FINAL
        gameobjects::GameObjectController::getInstance().renderDebug(elapsedTime, _debugFont);

        if (gameplay::Game::getInstance()->getConfig()->getBool("debug_draw_physics"))
        {
            gameplay::Game::getInstance()->getPhysicsController()->drawDebug(
                        gameobjects::GameObjectController::getInstance().getScene()->getActiveCamera()->getViewProjectionMatrix());
        }

        if (gameplay::Game::getInstance()->getConfig()->getBool("debug_show_fps"))
        {
            std::array<char, CHAR_MAX> buffer;
            sprintf(&buffer[0], "%d FPS", getFrameRate());
            _debugFont->start();
            static const int padding = 5;
            _debugFont->drawText(&buffer[0], padding, getHeight() - (padding * 4), gameplay::Vector4(1, 0, 0, 1));
            _debugFont->finish();
        }
#endif
    }

    void Platformer::renderSplashScreen(void *)
    {
        if(_splashScreenAlpha > 0.0f)
        {
            gameplay::Vector4 bgColor = gameplay::Vector4::fromColor(LevelRendererComponent::SKY_COLOR);
            bgColor.w = _splashScreenAlpha;

            _splashBackgroundSpriteBatch->start();
            _splashBackgroundSpriteBatch->draw(gameplay::Rectangle(0, 0, getWidth(), getHeight()),
                                               gameplay::Rectangle(0, 0, 1, 1),
                                               bgColor);
            _splashBackgroundSpriteBatch->finish();

            gameplay::Rectangle bounds(_splashForegroundSpriteBatch->getSampler()->getTexture()->getWidth(),
                                       _splashForegroundSpriteBatch->getSampler()->getTexture()->getHeight());
            gameplay::Vector2 drawPos;
            drawPos.x = (getWidth() / 2) - (bounds.width / 2);
            drawPos.y = (getHeight() / 2) - (bounds.height / 2);

            _splashForegroundSpriteBatch->start();
            _splashForegroundSpriteBatch->draw(gameplay::Rectangle (drawPos.x, drawPos.y, bounds.width, bounds.height),
                                               gameplay::Rectangle (0, 0, bounds.width, bounds.height),
                                               gameplay::Vector4(1, 1, 1, _splashScreenAlpha));
            _splashForegroundSpriteBatch->finish();

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

    void Platformer::setSplashScreenFade(float duration, bool isFadingIn)
    {
        static const float fadeInDirection = 1.0f;
        static const float fadeOutDirection = -1.0f;
        _splashScreenFadeRequests.push(std::make_pair(duration, isFadingIn ? fadeInDirection : fadeOutDirection));
    }
}
