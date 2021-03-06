#include "CameraControlComponent.h"

#include "GameObject.h"
#include "GameObjectController.h"
#include "Common.h"
#include "Messages.h"
#include "Scene.h"

namespace game
{
    CameraControlComponent::CameraControlComponent()
        : _camera(nullptr)
        , _previousZoom(0.0f)
        , _zoomSpeedScale(0.003f)
        , _smoothSpeedScale(0.25f)
        , _targetBoundaryScale(gameplay::Vector2(0.25f, 0.5))
        , _boundary(std::numeric_limits<float>::max(), std::numeric_limits<float>::max())
    {
        float const viewportWidth = 1280.0f;
        float const screenWidth =  gameplay::Game::getInstance()->getWidth();
        float const viewportZoom = GAME_UNIT_SCALAR * ((1.0f / screenWidth) * viewportWidth);
        _minZoom = viewportZoom / 2;
        _maxZoom = viewportZoom;
        _currentZoom = _maxZoom;
        _targetZoom = (_minZoom + _maxZoom) / 2;
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

    void CameraControlComponent::finalize()
    {
        SAFE_RELEASE(_camera);
    }

    void CameraControlComponent::update(gameplay::Vector2 const & target, gameplay::Vector2 const & velocity, float elapsedTime)
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

        GAME_ASSERT(!isnan(target.x) && !isnan(target.y), "Camera target is NaN!");

        // https://github.com/Andrea/SpringCamera2dXNA
        float const damping = 2.9f;
        float const springStiffness = 60;
        float const mass = 0.5f;
        gameplay::Vector2 const diff = _currentPosition - target;
        gameplay::Vector2 const force = -springStiffness * diff - damping * velocity;
        gameplay::Vector2 const acceleration = force / mass;
        float const dt = elapsedTime / 1000;
        _targetPosition += (velocity + (acceleration * dt)) * dt;
        _targetPosition.y = target.y;

        gameplay::Game::getInstance()->getAudioListener()->setPosition(_targetPosition.x, _targetPosition.y, 0.0);
        float const offsetX = (gameplay::Game::getInstance()->getWidth() / 2) *  _currentZoom;
        _targetPosition.x = MATH_CLAMP(_targetPosition.x, _boundary.x + offsetX, _boundary.x + _boundary.width - offsetX);
        float const offsetY = (gameplay::Game::getInstance()->getHeight() / 2) *  _currentZoom;
        _targetPosition.y = MATH_CLAMP(_targetPosition.y, _boundary.y + offsetY, std::numeric_limits<float>::max());
        _camera->getNode()->setTranslation(gameplay::Vector3(_targetPosition.x, _targetPosition.y, 0));
        _currentPosition.x = _camera->getNode()->getTranslationX();
        _currentPosition.y = _camera->getNode()->getTranslationY();
    }

    void CameraControlComponent::setBoundary(gameplay::Rectangle boundary)
    {
        _boundary = boundary;
    }

    void CameraControlComponent::setPosition(gameplay::Vector2 const & position)
    {
        _currentPosition = position;
        _camera->getNode()->setTranslation(gameplay::Vector3(_currentPosition.x, _currentPosition.y, 0));
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
