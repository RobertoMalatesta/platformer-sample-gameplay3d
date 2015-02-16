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
#include "ResourceManager.h"
#include "ScriptController.h"
#include "ScreenRenderer.h"
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
        , _renderLevelMessage(nullptr)
        , _optionsForm(nullptr)
#ifndef WIN32
        , _previousReleasedKey(gameplay::Keyboard::Key::KEY_NONE)
        , _framesSinceKeyReleaseEvent(0)
#endif
    {
    }

    Platformer::~Platformer()
    {
    }

    char const * PAUSE_TOGGLE_ID = "pauseToggle";
    char const * SOUND_TOGGLE_ID = "soundToggle";

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
#endif
        ResourceManager::getInstance().initializeForBoot();
        ScreenRenderer::getInstance().initialize();
        _optionsForm = gameplay::Form::create("res/ui/options.form");
        _optionsForm->getControl(PAUSE_TOGGLE_ID)->addListener(this, gameplay::Control::Listener::VALUE_CHANGED);
        _optionsForm->getControl(SOUND_TOGGLE_ID)->addListener(this, gameplay::Control::Listener::VALUE_CHANGED);

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
        gameobjects::GameObjectController::getInstance().registerComponent<SpriteAnimationComponent>("sprite_animation");
        gameobjects::GameObjectController::getInstance().registerComponent<CollisionHandlerComponent>("collision_handler");

        {
            PERF_SCOPE("GameObjectController::initialize");
            gameobjects::GameObjectController::getInstance().initialize();
        }

        _pinchMessage = PinchMessage::create();
        _keyMessage = KeyMessage::create();
        _touchMessage = TouchMessage::create();
        _mouseMessage = MouseMessage::create();
        _renderLevelMessage = RenderLevelMessage::create();

        getAudioListener()->setCamera(nullptr);
        gameobjects::GameObject * rootGameObject = gameobjects::GameObjectController::getInstance().createGameObject("root");
        gameobjects::GameObjectController::getInstance().createGameObject("level_0", rootGameObject);
    }

    void Platformer::finalize()
    {
#ifdef GP_USE_MEM_LEAK_DETECTION
        _optionsForm->getControl(PAUSE_TOGGLE_ID)->removeListener(this);
        _optionsForm->getControl(SOUND_TOGGLE_ID)->removeListener(this);
        gameobjects::GameObjectController::getInstance().finalize();
        GAMEOBJECTS_DELETE_MESSAGE(_pinchMessage);
        GAMEOBJECTS_DELETE_MESSAGE(_keyMessage);
        GAMEOBJECTS_DELETE_MESSAGE(_touchMessage);
        GAMEOBJECTS_DELETE_MESSAGE(_mouseMessage);
        GAMEOBJECTS_DELETE_MESSAGE(_renderLevelMessage);
        SAFE_DELETE(_optionsForm);
        ScreenRenderer::getInstance().finalize();
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
        if(_pinchMessage && !ScreenRenderer::getInstance().isVisible() && getState() != gameplay::Game::State::PAUSED)
        {
            PinchMessage::setMessage(_pinchMessage, x, y, scale);
            gameobjects::GameObjectController::getInstance().broadcastMessage(_pinchMessage);
        }
    }

    void Platformer::keyEvent(gameplay::Keyboard::KeyEvent evt, int key)
    {
        if (_keyMessage && !ScreenRenderer::getInstance().isVisible() && getState() != gameplay::Game::State::PAUSED)
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
        if (_touchMessage && !ScreenRenderer::getInstance().isVisible() && getState() != gameplay::Game::State::PAUSED)
        {
            TouchMessage::setMessage(_touchMessage, evt, x, y, contactIndex);
            gameobjects::GameObjectController::getInstance().broadcastMessage(_touchMessage);
        }
    }

    bool Platformer::mouseEvent(gameplay::Mouse::MouseEvent evt, int x, int y, int wheelDelta)
    {
        if (_mouseMessage && !ScreenRenderer::getInstance().isVisible() && getState() != gameplay::Game::State::PAUSED)
        {
            MouseMessage::setMessage(_mouseMessage, evt, x, y, wheelDelta);
            gameobjects::GameObjectController::getInstance().broadcastMessage(_mouseMessage);
        }
        
        return false;
    }

    void Platformer::update(float elapsedTime)
    {
        _optionsForm->update(elapsedTime);
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
        ScreenRenderer::getInstance().update(elapsedTime);
        _optionsForm->setEnabled(!ScreenRenderer::getInstance().isVisible());
    }

    void Platformer::render(float elapsedTime)
    {
        RenderLevelMessage::setMessage(_renderLevelMessage, elapsedTime);
        gameobjects::GameObjectController::getInstance().broadcastMessage(_renderLevelMessage);
        ScreenRenderer::getInstance().render();

        if (!ScreenRenderer::getInstance().isVisible())
        {
            if(getState() != gameplay::Game::State::PAUSED)
            {
                if (gameplay::Gamepad * gamepad = getGamepad(0))
                {
                    if (gameplay::Form * gamepadForm = gamepad->getForm())
                    {
                        gamepadForm->draw();
                    }
                }
            }

            _optionsForm->draw();
        }

#ifndef _FINAL
        std::array<char, CHAR_MAX> buffer;
        static const int paddingX = 5;
        static const gameplay::Vector4 textColor = gameplay::Vector4(1, 0, 0, 1);
        gameplay::Font * debugFont = ResourceManager::getInstance().getDebugFront();
        int const paddingY = debugFont->getSize();
        int y = paddingY / 2;
        debugFont->start();

        if (gameplay::Game::getInstance()->getConfig()->getBool("debug_show_fps"))
        {
            sprintf(&buffer[0], "%d FPS", getFrameRate());
            debugFont->drawText(&buffer[0], paddingX, y, textColor);
            y += paddingY;
        }

        if (gameplay::Game::getInstance()->getConfig()->getBool("debug_show_resolution"))
        {
            sprintf(&buffer[0], "%dx%d", getWidth(), getHeight());
            debugFont->drawText(&buffer[0], paddingX, y, textColor);
            y += paddingY;
        }

        debugFont->finish();
#endif
    }

    void Platformer::controlEvent(gameplay::Control * control, gameplay::Control::Listener::EventType)
    {
        if(strcmp(control->getId(), PAUSE_TOGGLE_ID) == 0)
        {
            // Pause is ref-counted and resume will be called when the game re-gains focus on mobile platforms. We
            // don't want to resume the game if the player had paused it when it did have focus so create a large ref-count.
            // Needs a better sollution
            bool const isPaused = getState() == gameplay::Game::State::PAUSED;
            for(int i = 0; i < 256; ++i)
            {
                 isPaused ? resume() : pause();
            }
        }
        else if(strcmp(control->getId(), SOUND_TOGGLE_ID) == 0)
        {
            getAudioListener()->setGain(getAudioListener()->getGain() == 1.0f ? 0.0f : 1.0f);
        }
    }
}
