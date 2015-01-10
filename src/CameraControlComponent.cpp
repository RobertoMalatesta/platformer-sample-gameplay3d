#include "CameraControlComponent.h"

#include "GameObject.h"
#include "GameObjectController.h"
#include "Common.h"
#include "MessagesPlatformer.h"

namespace platformer
{
    CameraControlComponent::CameraControlComponent()
        : _camera(nullptr)
        , _minZoom(PLATFORMER_UNIT_SCALAR / 2)
        , _maxZoom(PLATFORMER_UNIT_SCALAR * 2)
        , _currentZoom(_maxZoom)
        , _previousZoom(0.0f)
        , _targetZoom(PLATFORMER_UNIT_SCALAR)
        , _zoomSpeedScale(0.003f)
        , _smoothSpeedScale(0.25f)
        , _targetBoundaryScale(gameplay::Vector2(0.25f, 0.5))
        , _boundary(std::numeric_limits<float>::max(), std::numeric_limits<float>::max())
    {
    }

    CameraControlComponent::~CameraControlComponent()
    {
    }

    void CameraControlComponent::onStart()
    {
        _camera = gameobjects::GameObjectController::getInstance().getScene()->getActiveCamera();
        _camera->addRef();
        _initialCurrentZoom = _currentZoom;
        _initialTargetZoom = _targetZoom;
    }

    void CameraControlComponent::readProperties(gameplay::Properties & properties)
    {
        char const * targetBoundaryScaleId = "target_boundary_screen_scale";

        if(properties.exists(targetBoundaryScaleId))
        {
            properties.getVector2(targetBoundaryScaleId, &_targetBoundaryScale);
        }

        char const * zoomSpeedScaleId = "zoom_speed_scale";

        if(properties.exists(zoomSpeedScaleId))
        {
            _zoomSpeedScale = properties.getFloat(zoomSpeedScaleId);
        }

        char const * smoothSpeedScaleId = "smooth_speed_scale";

        if(properties.exists(smoothSpeedScaleId))
        {
            _smoothSpeedScale = properties.getFloat(smoothSpeedScaleId);
        }
    }

    void CameraControlComponent::onMessageReceived(gameplay::AIMessage * message)
    {
        switch (message->getId())
        {
        case(Messages::Type::PlatformerSplashScreenChangeRequestMessage):
            _currentZoom = _initialCurrentZoom;
            _targetZoom = _initialTargetZoom;
            break;
        }
    }

    void CameraControlComponent::update(float elapsedTime)
    {
        if(_currentZoom != _targetZoom)
        {
            _currentZoom = MATH_CLAMP(gameplay::Curve::lerp(elapsedTime * _zoomSpeedScale, _currentZoom, _targetZoom) , _minZoom, _maxZoom);

            if(_currentZoom != _previousZoom)
            {
                _camera->setZoomX(gameplay::Game::getInstance()->getWidth() * _currentZoom);
                _camera->setZoomY(gameplay::Game::getInstance()->getHeight() * _currentZoom);
            }

            _previousZoom = _currentZoom;
        }
    }

    void CameraControlComponent::finalize()
    {
        SAFE_RELEASE(_camera);
    }

    void CameraControlComponent::setTargetPosition(gameplay::Vector2 const & target, float elapsedTime)
    {
        _targetPosition = target;
        gameplay::Game::getInstance()->getAudioListener()->setPosition(_targetPosition.x, _targetPosition.y, 0.0);
        float const offsetX = (gameplay::Game::getInstance()->getWidth() / 2) *  _currentZoom;
        _targetPosition.x = MATH_CLAMP(_targetPosition.x, _boundary.x + offsetX, _boundary.x + _boundary.width - offsetX);
        float const offsetY = (gameplay::Game::getInstance()->getHeight() / 2) *  _currentZoom;
        _targetPosition.y = MATH_CLAMP(_targetPosition.y, _boundary.y + offsetY, _boundary.y + _boundary.height + offsetY);
        _camera->getNode()->setTranslation(gameplay::Vector3(_targetPosition.x, _targetPosition.y, 0));
        _currentPosition.x = _camera->getNode()->getTranslationX();
        _currentPosition.y = _camera->getNode()->getTranslationY();
    }

    void CameraControlComponent::setBoundary(gameplay::Rectangle boundary)
    {
        _boundary = boundary;
    }

    float CameraControlComponent::getMinZoom() const
    {
        return _minZoom;
    }

    float CameraControlComponent::getMaxZoom() const
    {
        return _maxZoom;
    }

    float CameraControlComponent::getZoom() const
    {
        return _currentZoom;
    }

    float CameraControlComponent::getTargetZoom() const
    {
        return _targetZoom;
    }

    gameplay::Matrix const & CameraControlComponent::getViewProjectionMatrix() const
    {
        return _camera->getViewProjectionMatrix();
    }

    void CameraControlComponent::setZoom(float zoom)
    {
        _targetZoom = zoom;
    }

    gameplay::Vector2 const & CameraControlComponent::getPosition() const
    {
        return _currentPosition;
    }

    gameplay::Rectangle const & CameraControlComponent::getTargetBoundary() const
    {
        return _boundary;
    }

    gameplay::Vector2 const & CameraControlComponent::getTargetPosition() const
    {
        return _targetPosition;
    }
}
