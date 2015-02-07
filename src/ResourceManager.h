#ifndef GAME_RESOURCE_MANAGER
#define GAME_RESOURCE_MANAGER

#include <map>
#include <string>
#include <vector>
#include "Ref.h"

namespace gameplay
{
    class Properties;
    class Texture;
    class SpriteBatch;

#ifndef _FINAL
    class Font;
#endif
}

namespace game
{
    class PropertiesRef;
    class SpriteSheet;

    /** @script{ignore} */
    class ResourceManager
    {
    public:
        static ResourceManager & getInstance();

        void initializeForBoot();
        void initialize();
        void finalize();

        gameplay::SpriteBatch * createSinglePixelSpritebatch();
        PropertiesRef * getProperties(std::string const & url);
        SpriteSheet * getSpriteSheet(std::string const & url);
#ifndef _FINAL
        gameplay::Font * getDebugFront() const;
#endif
    private:
        explicit ResourceManager();
        ~ResourceManager();
        ResourceManager(ResourceManager const &);

        std::vector<gameplay::Texture *> _cachedTextures;
        std::map<std::string, PropertiesRef *> _cachedProperties;
        std::map<std::string, SpriteSheet *> _cachedSpriteSheets;

#ifndef _FINAL
        gameplay::Font * _debugFont;
#endif
    };
}

#endif
