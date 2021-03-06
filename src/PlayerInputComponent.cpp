#include "PlayerInputComponent.h"

#include "CameraControlComponent.h"
#include "Common.h"
#include "Game.h"
#include "GameObject.h"
#include "ScreenRenderer.h"

#ifndef _FINAL
#include "Messages.h"
#endif

namespace game
{
    PlayerInputComponent::PlayerInputComponent()
        : _pinchEnabled(true)
    {
    }

    PlayerInputComponent::~PlayerInputComponent()
    {
    }

    void PlayerInputComponent::onStart()
    {
        _player = getParent()->getComponent<PlayerComponent>();
        _player->addRef();
        _camera = getRootParent()->getComponent<CameraControlComponent>();
        _camera->addRef();
    }

    void PlayerInputComponent::initialize()
    {
        _gamepadButtonMapping[GamepadButtons::Jump] = gameplay::Gamepad::BUTTON_A;
#ifndef _FINAL
        _gamepadButtonMapping[GamepadButtons::Reload] = gameplay::Gamepad::BUTTON_MENU1;
#endif
    }

    void PlayerInputComponent::finalize()
    {
        SAFE_RELEASE(_player);
        SAFE_RELEASE(_camera);
    }

    void PlayerInputComponent::update()
    {
        _gamePad = gameplay::Game::getInstance()->getGamepad(0);

        if (_gamePad && !ScreenRenderer::getInstance().isVisible())
        {
            bool isAnyButtonDown = false;

            for(int i = 0; i < GamepadButtons::EnumCount; ++i)
            {
                bool const isButtonDown = _gamePad->isButtonDown(static_cast<gameplay::Gamepad::ButtonMapping>(_gamepadButtonMapping[i]));
                _gamepadButtonState[i] = isButtonDown;
                isAnyButtonDown |= isButtonDown;
            }

            if(isGamepadButtonPressed(GamepadButtons::Jump))
            {
                _player->jump(PlayerComponent::JumpSource::Input);
            }
#ifndef _FINAL
            else if (isGamepadButtonPressed(GamepadButtons::Reload))
            {
                gameplay::AIMessage * reloadMsg = RequestLevelReloadMessage::create();
                getRootParent()->broadcastMessage(reloadMsg);
                GAMEOBJECTS_DELETE_MESSAGE(reloadMsg);
            }
#endif

            gameplay::Vector2 joystickValue;
            _gamePad->getJoystickValues(0, &joystickValue);

            if(_previousJoystickValue != joystickValue)
            {
                _player->setMovementEnabled(PlayerComponent::MovementDirection::Left, joystickValue.x < 0, fabs(joystickValue.x));
                _player->setMovementEnabled(PlayerComponent::MovementDirection::Right, joystickValue.x > 0, fabs(joystickValue.x));
                _player->setMovementEnabled(PlayerComponent::MovementDirection::Down, joystickValue.y < 0, fabs(joystickValue.y));
                _player->setMovementEnabled(PlayerComponent::MovementDirection::Up, joystickValue.y > 0, fabs(joystickValue.y));
            }

            _pinchEnabled = joystickValue.isZero() || !isAnyButtonDown;
            _previousGamepadButtonState = _gamepadButtonState;
            _previousJoystickValue = joystickValue;
        }
    }

    bool PlayerInputComponent::isGamepadButtonPressed(GamepadButtons::Enum button) const
    {
        return _gamepadButtonState[button] && !_previousGamepadButtonState[button];
    }

    bool PlayerInputComponent::isGamepadButtonReleased(GamepadButtons::Enum button) const
    {
        return !_gamepadButtonState[button] && _previousGamepadButtonState[button];
    }

    bool PlayerInputComponent::onMessageReceived(gameobjects::Message * message, int messageType)
    {
        switch (messageType)
        {
        case(Messages::Type::Key) :
            onKeyboardInput(std::move(KeyMessage(message)));
            break;
        case(Messages::Type::Mouse):
            onMouseInput(std::move(MouseMessage(message)));
            break;
        case(Messages::Type::Pinch):
            onPinchInput(std::move(PinchMessage(message)));
            break;
        }

        return true;
    }

    float PlayerInputComponent::calculateCameraZoomStep(float scale) const
    {
        float const zoomDelta = ((_camera->getMaxZoom() - _camera->getMinZoom()) / 5.0f) * -scale;
        return _camera->getZoom() + zoomDelta;
    }

    void PlayerInputComponent::onMouseInput(MouseMessage mouseMessage)
    {
        if(mouseMessage._wheelDelta != 0)
        {
            _camera->setZoom(calculateCameraZoomStep(mouseMessage._wheelDelta));
        }
    }

    void PlayerInputComponent::onPinchInput(PinchMessage pinchMessage)
    {
        if(_pinchEnabled)
        {
            static float const zoomFactor = -100.0f;
            float const scale = (1.0f - pinchMessage._scale) * zoomFactor;
            _camera->setZoom(calculateCameraZoomStep(scale));
        }
    }

    void PlayerInputComponent::onKeyboardInput(KeyMessage keyMessage)
    {   
        if (keyMessage._event == gameplay::Keyboard::KeyEvent::KEY_PRESS ||
            keyMessage._event == gameplay::Keyboard::KeyEvent::KEY_RELEASE)
        {
            bool const enable = keyMessage._event == gameplay::Keyboard::KeyEvent::KEY_PRESS;

            bool const keyWasDown = _keyState[keyMessage._key];
            _keyState[keyMessage._key] = keyMessage._event == gameplay::Keyboard::KeyEvent::KEY_PRESS;

            if (!enable || !keyWasDown)
            {
                switch (keyMessage._key)
                {
                case gameplay::Keyboard::Key::KEY_LEFT_ARROW:
                case gameplay::Keyboard::Key::KEY_A:
                    _player->setMovementEnabled(PlayerComponent::MovementDirection::Left, enable);
                    break;
                case gameplay::Keyboard::Key::KEY_RIGHT_ARROW:
                case gameplay::Keyboard::Key::KEY_D:
                    _player->setMovementEnabled(PlayerComponent::MovementDirection::Right, enable);
                    break;
                case gameplay::Keyboard::Key::KEY_UP_ARROW:
                case gameplay::Keyboard::Key::KEY_W:
                    _player->setMovementEnabled(PlayerComponent::MovementDirection::Up, enable);
                    break;
                case gameplay::Keyboard::Key::KEY_DOWN_ARROW:
                case gameplay::Keyboard::Key::KEY_S:
                    _player->setMovementEnabled(PlayerComponent::MovementDirection::Down, enable);
                    break;
                case gameplay::Keyboard::Key::KEY_SPACE:
                    if (enable)
                    {
                        _player->jump(PlayerComponent::JumpSource::Input);
                    }
                    break;
                case gameplay::Keyboard::Key::KEY_PG_UP:
                case gameplay::Keyboard::Key::KEY_PG_DOWN:
                {
                    if (enable)
                    {
                        float inputScale = keyMessage._key == gameplay::Keyboard::Key::KEY_PG_UP ? 1.0f : -1.0f;
                        _camera->setZoom(calculateCameraZoomStep(inputScale));
                    }
                }
                break;
                }
            }
        }
    }
}
