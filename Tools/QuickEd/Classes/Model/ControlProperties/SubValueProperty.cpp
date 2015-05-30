#include "SubValueProperty.h"

#include "ValueProperty.h"

using namespace DAVA;

SubValueProperty::SubValueProperty(int anIndex, const DAVA::String &propName)
    : index(anIndex)
    , name(propName)
{
    
}

SubValueProperty::~SubValueProperty()
{
    
}

int SubValueProperty::GetCount() const
{
    return 0;
}

AbstractProperty *SubValueProperty::GetProperty(int index) const
{
    return NULL;
}

void SubValueProperty::Accept(PropertyVisitor *visitor)
{
    DVASSERT(false);
}

const DAVA::String &SubValueProperty::GetName() const
{
    return name;
}

SubValueProperty::ePropertyType SubValueProperty::GetType() const
{
    return TYPE_VARIANT;
}

VariantType SubValueProperty::GetValue() const
{
    return GetValueProperty()->GetSubValue(index);
}

void SubValueProperty::SetValue(const DAVA::VariantType &newValue)
{
    GetValueProperty()->SetSubValue(index, newValue);
}

VariantType SubValueProperty::GetDefaultValue() const
{
    return GetValueProperty()->GetDefaultSubValue(index);
}

void SubValueProperty::SetDefaultValue(const DAVA::VariantType &newValue)
{
    GetValueProperty()->SetDefaultSubValue(index, newValue);
}

void SubValueProperty::ResetValue()
{
    GetValueProperty()->ResetValue();
}

bool SubValueProperty::IsReplaced() const
{
    return GetValueProperty()->IsReplaced();
}

ValueProperty *SubValueProperty::GetValueProperty() const
{
    return DynamicTypeCheck<ValueProperty*>(GetParent());
}
