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
        , _targetZoom(PLATFORMER_UNIT_SCALAR)
        , _zoomSpeedScale(0.003f)
        , _smoothSpeedScale(0.003f)
        , _targetBoundaryScale(gameplay::Vector2(0.25f, 0.5))
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
            _camera->setZoomX(gameplay::Game::getInstance()->getWidth() * _currentZoom);
            _camera->setZoomY(gameplay::Game::getInstance()->getHeight() * _currentZoom);
        }

        gameplay::Vector2 scaledViewport(gameplay::Game::getInstance()->getWidth() * _currentZoom,
                                         gameplay::Game::getInstance()->getHeight() * _currentZoom);
        _targetBoundary.width = scaledViewport.x * _targetBoundaryScale.x;
        _targetBoundary.height = scaledViewport.y * _targetBoundaryScale.y;
        _targetBoundary.x = _currentPosition.x - (_targetBoundary.width / 2.0f);
        _targetBoundary.y = _currentPosition.y - (_targetBoundary.height/ 2.0f);

        float const scaledDimension = getPositionIntersectionDimension() * PLATFORMER_UNIT_SCALAR;
        float smoothScale = _smoothSpeedScale;

        if(!_targetBoundary.intersects(_targetPosition.x, _targetPosition.y, scaledDimension, scaledDimension))
        {
            smoothScale *= 5.0f;
        }
        _currentPosition.x = gameplay::Curve::lerp(elapsedTime * smoothScale, _currentPosition.x, _targetPosition.x);
        _currentPosition.y = gameplay::Curve::lerp(elapsedTime * smoothScale, _currentPosition.y, _targetPosition.y);
        _camera->getNode()->setTranslation(gameplay::Vector3(_currentPosition.x, _currentPosition.y, 0.0f));
    }

    void CameraControlComponent::finalize()
    {
        SAFE_RELEASE(_camera);
    }

    void CameraControlComponent::setTargetPosition(gameplay::Vector2 const & target)
    {
        _targetPosition = target;
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

    float CameraControlComponent::setZoom(float zoom)
    {
        return _targetZoom = zoom;
    }

    gameplay::Vector2 const & CameraControlComponent::getPosition() const
    {
        return _currentPosition;
    }

    gameplay::Rectangle const & CameraControlComponent::getTargetBoundary() const
    {
        return _targetBoundary;
    }

    gameplay::Vector2 const & CameraControlComponent::getTargetPosition() const
    {
        return _targetPosition;
    }

    float CameraControlComponent::getPositionIntersectionDimension() const
    {
        return 10.0f;
    }
}
