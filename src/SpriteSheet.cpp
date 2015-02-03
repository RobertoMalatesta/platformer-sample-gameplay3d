#include "SpriteSheet.h"

#include "Common.h"

namespace platformer
{
    SpriteSheet * SpriteSheet::create(std::string const & spriteSheetPath)
    {
        SpriteSheet * spriteSheet = nullptr;
        
        auto & cache = getCache();

        auto itr = cache.find(spriteSheetPath);

        if (itr != cache.end())
        {
            itr->second->addRef();
            spriteSheet = itr->second;
        }
        else
        {
            spriteSheet = new SpriteSheet();
            spriteSheet->initialize(spriteSheetPath);
            cache[spriteSheetPath] = spriteSheet;
        }

        return spriteSheet;
    }

    SpriteSheet::SpriteSheet()
        : _texture(nullptr)
    {
    }

    SpriteSheet::~SpriteSheet()
    {
        SAFE_RELEASE(_texture);
    }

    void SpriteSheet::initialize(std::string const & filePath)
    {
        PropertiesRef * propertyRef = createProperties(filePath.c_str());
        gameplay::Properties * properties = propertyRef->get();

        PLATFORMER_ASSERT(properties, "Failed to load sprite sheet %s", filePath.c_str());

        while (gameplay::Properties * currentNamespace = properties->getNextNamespace())
        {
            if (strcmp(currentNamespace->getNamespace(), "frames") == 0)
            {
                while (gameplay::Properties * frameNamespace = currentNamespace->getNextNamespace())
                {
                    Sprite sprite;
                    sprite._isRotated = frameNamespace->getBool("rotated");
                    sprite._name = frameNamespace->getString("filename");

                    while (gameplay::Properties * frameChildNamespace = frameNamespace->getNextNamespace())
                    {
                        if (strcmp(frameChildNamespace->getNamespace(), "frame") == 0)
                        {
                            sprite._src.width = frameChildNamespace->getInt("w");
                            sprite._src.height = frameChildNamespace->getInt("h");
                            sprite._src.x = frameChildNamespace->getInt("x");
                            sprite._src.y = frameChildNamespace->getInt("y");
                        }
                    }

                    PLATFORMER_ASSERT(_sprites.find(sprite._name) == _sprites.end(),
                        "Duplicate sprite name '%s' in '%s'", sprite._name.c_str(), filePath.c_str());

                    _sprites[sprite._name] = sprite;
                }
            }
            else if (strcmp(currentNamespace->getNamespace(), "meta") == 0)
            {
                std::string const texturePath = currentNamespace->getString("image");
                _texture = gameplay::Texture::create(texturePath.c_str());
                gameplay::Properties * dimensionsNamespace = currentNamespace->getNextNamespace();
                int const width = dimensionsNamespace->getInt("w");
                int const height = dimensionsNamespace->getInt("h");
                PLATFORMER_ASSERT(_texture && _texture->getWidth() == width && _texture->getHeight() == height,
                    "Spritesheet '%s' width/height meta texture meta data is incorrect for '%s'", filePath.c_str(), texturePath.c_str());
            }
        }

        SAFE_RELEASE(propertyRef);
    }

    Sprite * SpriteSheet::getSprite(std::string const & spriteName)
    {
        auto itr = _sprites.find(spriteName);
        return itr != _sprites.end() ? &itr->second : nullptr;
    }

    gameplay::Rectangle const & SpriteSheet::getSrc()
    {
        return _src;
    }

    gameplay::Texture * SpriteSheet::getTexture()
    {
        return _texture;
    }

    std::string const & SpriteSheet::getName() const
    {
        return _name;
    }

    std::map<std::string, SpriteSheet *> & SpriteSheet::getCache()
    {
        static std::map<std::string, SpriteSheet *> cache;
        return cache;
    }

    void SpriteSheet::forEachSprite(std::function<void(Sprite const &)> func)
    {
        for(auto & spritePair : _sprites)
        {
            func(spritePair.second);
        }
    }
}
