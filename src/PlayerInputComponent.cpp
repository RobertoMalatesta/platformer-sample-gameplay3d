#include "PlayerInputComponent.h"

#include "CameraControlComponent.h"
#include "CameraControlComponent.h"
#include "Common.h"
#include "GameObject.h"
#include "GameObjectController.h"

namespace platformer
{
    PlayerInputComponent::PlayerInputComponent()
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
        _previousGamepadButtonState.fill(false);
        _gamepadButtonState.fill(false);
        _joystickMovementDirections[PlayerComponent::MovementDirection::Up] = gameplay::Vector2(0, 1);
        _joystickMovementDirections[PlayerComponent::MovementDirection::Down] = gameplay::Vector2(0, -1);
        _joystickMovementDirections[PlayerComponent::MovementDirection::Left] = gameplay::Vector2(-1, 0);
        _joystickMovementDirections[PlayerComponent::MovementDirection::Right] = gameplay::Vector2(1, 0);
        _gamepadButtonMapping[GamepadButtons::Jump] = gameplay::Gamepad::BUTTON_A;
        _gamePad = gameplay::Game::getInstance()->getGamepad(0);
    }

    void PlayerInputComponent::finalize()
    {
        SAFE_RELEASE(_player);
        SAFE_RELEASE(_camera);
    }

    void PlayerInputComponent::update(float)
    {
        for(int i = 0; i < GamepadButtons::EnumCount; ++i)
        {
            _gamepadButtonState[i] =_gamePad->isButtonDown(static_cast<gameplay::Gamepad::ButtonMapping>(_gamepadButtonMapping[i]));
        }

        if(isGamepadButtonPressed(GamepadButtons::Jump))
        {
            _player->jump();
        }

        gameplay::Vector2 joystickValue;
        _gamePad->getJoystickValues(0, &joystickValue);

        if(_previousJoystickValue != joystickValue)
        {
            gameplay::Vector2 joystickValueUnit = joystickValue;
            joystickValueUnit.normalize();

            float const maxDirectionDelta = cos(MATH_DEG_TO_RAD(25));

            for(int i = PlayerComponent::MovementDirection::None + 1; i < PlayerComponent::MovementDirection::EnumCount; ++i)
            {
                _player->setMovementEnabled(static_cast<PlayerComponent::MovementDirection::Enum>(i),
                                         joystickValueUnit.dot(_joystickMovementDirections[i]) > maxDirectionDelta,
                                         joystickValue.length());
            }
        }

        _previousGamepadButtonState = _gamepadButtonState;
        _previousJoystickValue = joystickValue;
    }

    bool PlayerInputComponent::isGamepadButtonPressed(GamepadButtons::Enum button) const
    {
        return _gamepadButtonState[button] && !_previousGamepadButtonState[button];
    }

    bool PlayerInputComponent::isGamepadButtonReleased(GamepadButtons::Enum button) const
    {
        return !_gamepadButtonState[button] && _previousGamepadButtonState[button];
    }

    gameplay::Form * PlayerInputComponent::getGamepadForm() const
    {
        return _gamePad->getForm();
    }

    void PlayerInputComponent::onMessageReceived(gameplay::AIMessage * message)
    {
        switch (message->getId())
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
    }

    float PlayerInputComponent::calculateCameraZoomStep(bool increase) const
    {
        float const zoomDelta = ((_camera->getMaxZoom() - _camera->getMinZoom()) / 10.0f) * (increase ? -1.0f : 1.0f);
        return _camera->getZoom() + zoomDelta;
    }

    void PlayerInputComponent::onMouseInput(MouseMessage mouseMessage)
    {
        if(mouseMessage._wheelDelta != 0)
        {
            _camera->setZoom(calculateCameraZoomStep(mouseMessage._wheelDelta > 0));
        }
    }

    void PlayerInputComponent::onPinchInput(PinchMessage pinchMessage)
    {
    }

    void PlayerInputComponent::onKeyboardInput(KeyMessage keyMessage)
    {   
        if (keyMessage._event == gameplay::Keyboard::KeyEvent::KEY_PRESS ||
            keyMessage._event == gameplay::Keyboard::KeyEvent::KEY_RELEASE)
        {
            bool const enable = keyMessage._event == gameplay::Keyboard::KeyEvent::KEY_PRESS;

            switch(keyMessage._key)
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
                if(enable)
                {
                    _player->jump();
                }
                break;
            case gameplay::Keyboard::Key::KEY_PG_UP:
            case gameplay::Keyboard::Key::KEY_PG_DOWN:
                {
                    if(enable)
                    {
                        _camera->setZoom(calculateCameraZoomStep(keyMessage._key == gameplay::Keyboard::Key::KEY_PG_UP));
                    }
                }
                break;
            }
        }
    }
}
