#include "nodes/mec/MEPlatform/MeServices/Resources/AttributeBase.h"

AttributeBase::AttributeBase()
{
}
AttributeBase::~AttributeBase()
{
}

std::string AttributeBase::toJson( const std::string& value )
{
    return value;
}

std::string AttributeBase::toJson( const std::time_t& value )
{
    char buf[sizeof "2011-10-08T07:07:09Z"];
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&value));
    return buf;
}

int32_t AttributeBase::toJson( int32_t value )
{
    return value;
}

int64_t AttributeBase::toJson( int64_t value )
{
    return value;
}

double AttributeBase::toJson( double value )
{
    return value;
}

bool AttributeBase::toJson( bool value )
{
    return value;
}

nlohmann::ordered_json AttributeBase::toJson( AttributeBase&  content )
{
    return  content.toJson();
}
