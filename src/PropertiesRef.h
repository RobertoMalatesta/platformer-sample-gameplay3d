#ifndef GAME_PROPERTIES_REF
#define GAME_PROPERTIES_REF

#include "Ref.h"

namespace gameplay
{
    class Properties;
}

namespace game
{
    /** @script{ignore} */
    class PropertiesRef : public gameplay::Ref
    {
        friend class ResourceManager;

    public:
        ~PropertiesRef();
        gameplay::Properties * get();
    private:
        PropertiesRef(gameplay::Properties * properties);

        gameplay::Properties * _properties;
    };
}

#endif
