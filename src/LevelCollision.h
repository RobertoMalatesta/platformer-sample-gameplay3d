#ifndef GAME_TERRAIN_INFO_H
#define GAME_TERRAIN_INFO_H

#include "GameObjectCommon.h"

namespace game
{
    /**
     * The unique tile types that can be set per tile in a level.
     *
     * @script{ignore}
    */
    struct CollisionType
    {
        enum Enum
        {
            COLLISION_STATIC,
            COLLISION_DYNAMIC,
            LADDER,
            RESET,
            COLLECTABLE,
            WATER,
            BRIDGE
        };
    };

    /** @script{ignore} */
    class NodeCollisionInfo : public gameobjects::INodeUserData
    {
    public:
        static NodeCollisionInfo * getNodeCollisionInfo(gameplay::Node * node);

        int getNodeUserDataId() const override;

        CollisionType::Enum _CollisionType;
    };
}

#endif
