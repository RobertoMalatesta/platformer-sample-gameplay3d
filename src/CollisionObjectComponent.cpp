#include "CollisionObjectComponent.h"

#include "GameObject.h"
#include "Common.h"
#include "PropertiesRef.h"
#include "ResourceManager.h"

namespace game
{
    CollisionObjectComponent::CollisionObjectComponent()
        : _node(nullptr)
    {
    }

    CollisionObjectComponent::~CollisionObjectComponent()
    {
    }

    void CollisionObjectComponent::initialize()
    {
        PropertiesRef * physicsPropertiesRef = ResourceManager::getInstance().getProperties(_physics.c_str());
        gameplay::Properties * physicsProperties = physicsPropertiesRef->get();
        _node = gameplay::Node::create(getId().c_str());
        _node->setCollisionObject(_physics.c_str());

        if (physicsProperties->exists("extents"))
        {
            gameplay::Vector3 scale;
            physicsProperties->getVector3("extents", &scale);
            _node->setScale(scale);
        }

        getParent()->getNode()->addChild(_node);
        SAFE_RELEASE(physicsPropertiesRef);
    }

    void CollisionObjectComponent::finalize()
    {
        getParent()->getNode()->removeChild(_node);
        SAFE_RELEASE(_node);
    }

    void CollisionObjectComponent::readProperties(gameplay::Properties & properties)
    {
        _physics = properties.getString("physics");
    }

    gameplay::Node * CollisionObjectComponent::getNode() const
    {
        return _node;
    }
}
