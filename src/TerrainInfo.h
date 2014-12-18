#ifndef PLATFORMER_TERRAIN_INFO_H
#define PLATFORMER_TERRAIN_INFO_H

#include "Common.h"
#include "GameObjectCommon.h"

namespace platformer
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
            COLLISION,
            LADDER,
            RESET
        };
    };

    /** @script{ignore} */
    class TerrainInfo : public gameobjects::INodeUserData
    {
    public:
        static TerrainInfo * getTerrainInfo(gameplay::Node * node);

        int getNodeUserDataId() const override;

        CollisionType::Enum _CollisionType;
    };
}

#endif
