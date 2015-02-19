#ifndef GAME_CAMERA_COMPONENT_H
#define GAME_CAMERA_COMPONENT_H

#include "Component.h"

namespace game
{
    /**
     * Controls the active camera, any smoothing/zooming etc should be applied by and configured via this component
     *
     * @script{ignore}
    */
    class CameraControlComponent : public gameobjects::Component
    {
    public:
        explicit CameraControlComponent();
        ~CameraControlComponent();

        void setTargetPosition(gameplay::Vector2 const & target, float);
        float getZoom() const;
        float getTargetZoom() const;
        float getMinZoom() const;
        float getMaxZoom() const;
        void setZoom(float zoom);
        void setBoundary(gameplay::Rectangle boundary);
        void update(float elapsedTime);
        gameplay::Matrix const & getViewProjectionMatrix() const;
        gameplay::Vector2 const &  getPosition() const;
        gameplay::Vector2 const & getTargetPosition() const;
        gameplay::Rectangle const & getTargetBoundary() const;
    protected:
        virtual void readProperties(gameplay::Properties & properties) override;
        virtual void onStart() override;
        virtual void finalize() override;
    private:
        CameraControlComponent(CameraControlComponent const &);

        gameplay::Camera * _camera;
        float _minZoom;
        float _maxZoom;
        float _initialCurrentZoom;
        float _initialTargetZoom;
        float _currentZoom;
        float _previousZoom;
        float _targetZoom;
        float _zoomSpeedScale;
        float _smoothSpeedScale;
        gameplay::Rectangle _boundary;
        gameplay::Vector2 _targetBoundaryScale;
        gameplay::Vector2 _targetPosition;
        gameplay::Vector2 _currentPosition;
    };
}

#endif
