#include "LevelCollision.h"
#include "GameObject.h"

namespace game
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
