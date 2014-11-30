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
            _currentZoom = MATH_CLAMP(gameplay::Curve::lerp(elapsedTime / 333.0f, _currentZoom, _targetZoom) , _minZoom, _maxZoom);
            _camera->setZoomX(gameplay::Game::getInstance()->getWidth() * _currentZoom);
            _camera->setZoomY(gameplay::Game::getInstance()->getHeight() * _currentZoom);
        }
    }

    void CameraControlComponent::finalize()
    {
        SAFE_RELEASE(_camera);
    }

    void CameraControlComponent::setTargetPosition(gameplay::Vector2 const & target)
    {
        _camera->getNode()->setTranslation(std::move(gameplay::Vector3(target.x, target.y, 0.0f)));
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
}
