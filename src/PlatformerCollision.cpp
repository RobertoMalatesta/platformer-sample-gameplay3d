#include "PlatformerCollision.h"
#include "GameObject.h"

namespace platformer
{
    static const int COLLISION_TYPE_ID = gameobjects::GameObject::GAMEOBJECT_NODE_USER_DATA_ID + 1;

    NodeCollisionInfo * NodeCollisionInfo::getNodeCollisionInfo(gameplay::Node * node)
    {
        return gameobjects::getNodeUserData<NodeCollisionInfo>(node, COLLISION_TYPE_ID);
    }

    int NodeCollisionInfo::getNodeUserDataId() const
    {
        return COLLISION_TYPE_ID;
    }
}
