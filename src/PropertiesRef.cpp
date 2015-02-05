#include "PropertiesRef.h"

#include "Common.h"
#include "Properties.h"

namespace game
{
    PropertiesRef::PropertiesRef(gameplay::Properties * properties)
        : _properties(properties)
    {
    }

    PropertiesRef::~PropertiesRef()
    {
        SAFE_DELETE(_properties);
    }

    gameplay::Properties * PropertiesRef::get()
    {
        return _properties;
    }
}
