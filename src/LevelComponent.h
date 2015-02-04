#ifndef PLATFORMER_LEVEL_COMPONENT_H
#define PLATFORMER_LEVEL_COMPONENT_H

#include "Component.h"
#include "PlatformerCollision.h"

namespace gameplay
{
    class Properties;
}

namespace platformer
{
    /**
     * Creates the player, enemy AI and physics defined in a level file (*.level).
     *
     * Level files are created in [Tiled], exported as JSON and then converted to the
     * gameplay property format using [Json2gp3d]
     *
     * [Tiled]      Download @ http://www.mapeditor.org/download.html
     * [Json2gp3d]  Download @ https://github.com/louis-mclaughlin/json-to-gameplay3d
     *
     * @script{ignore}
    */
    class LevelComponent : public gameobjects::Component
    {
    public:
        static int const EMPTY_TILE = 0;

        explicit LevelComponent();
        ~LevelComponent();

        virtual void initialize() override;
        virtual void finalize() override;
        virtual void onMessageReceived(gameobjects::Message * message, int messageType) override;
        virtual void readProperties(gameplay::Properties & properties) override;
        virtual void update(float) override;

        void consumeCollectable(gameplay::Node * collectableNode);
        void reload();
        std::string const & getTexturePath() const;
        int getTileWidth() const;
        int getTileHeight() const;
        int getTile(int x, int y) const;
        int getWidth() const;
        int getHeight() const;
        gameplay::Vector2 const & getPlayerSpawnPosition() const;

        struct Collectable
        {
            gameplay::Rectangle _src;
            gameplay::Node * _node;
            gameplay::Vector3 _startPosition;
            bool _active;
            bool _visible;
        };

        void getCollectables(std::vector<Collectable*> & collectablesOut);
        void forEachCachedNode(CollisionType::Enum terrainType, std::function<void(gameplay::Node *)> func);
    private:
        struct Tile
        {
            Tile() : _tileId(0) {}
            int _tileId;
        };

        LevelComponent(LevelComponent const &);

        void load();
        void loadTerrain(gameplay::Properties * layerNamespace);
        void loadCharacters(gameplay::Properties * layerNamespace);
        void loadStaticCollision(gameplay::Properties * layerNamespace, CollisionType::Enum terrainType);
        void loadDynamicCollision(gameplay::Properties * layerNamespace);
        void loadCollectables(gameplay::Properties * layerNamespace);
        void loadBridges(gameplay::Properties * layerNamespace);
        void unload();

        gameplay::Rectangle getObjectBounds(gameplay::Properties * objectNamespace) const;
        gameplay::Node * createCollisionObject(CollisionType::Enum collisionType, gameplay::Properties * collisionProperties, gameplay::Rectangle const & bounds, float rotationZ = 0.0f);
        void placeEnemies();

        std::string _level;
        std::string _texturePath;
        int _width;
        int _height;
        int _tileWidth;
        int _tileHeight;
        bool _loadBroadcasted;
        gameplay::Vector2 _playerSpawnPosition;
        std::vector<std::vector<Tile>> _grid;
        gameobjects::Message * _loadedMessage;
        gameobjects::Message * _unloadedMessage;
        gameobjects::Message * _preUnloadedMessage;
        std::vector<gameobjects::GameObject*> _children;
        std::map <CollisionType::Enum, std::vector<gameplay::Node*>> _collisionNodes;
        std::map<gameplay::Node *, Collectable> _collectables;
    };
}

#endif
