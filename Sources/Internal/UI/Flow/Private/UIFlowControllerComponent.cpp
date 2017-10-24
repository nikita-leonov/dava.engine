#include "UI/Flow/UIFlowControllerComponent.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIFlowControllerComponent)
{
    ReflectionRegistrator<UIFlowControllerComponent>::Begin()
    .ConstructorByPointer()
    .DestructorByPointer([](UIFlowControllerComponent* c) { SafeRelease(c); })
    .Field("reflectionTypeName", &UIFlowControllerComponent::GetReflectionTypeName, &UIFlowControllerComponent::SetReflectionTypeName)
    .Field("luaScriptPath", &UIFlowControllerComponent::GetLuaScriptPath, &UIFlowControllerComponent::SetLuaScriptPath)
    .End();
}

UIFlowControllerComponent::UIFlowControllerComponent() = default;

UIFlowControllerComponent::UIFlowControllerComponent(const UIFlowControllerComponent& dst) = default;

UIFlowControllerComponent::~UIFlowControllerComponent() = default;

UIFlowControllerComponent* UIFlowControllerComponent::Clone() const
{
    return new UIFlowControllerComponent(*this);
}

void UIFlowControllerComponent::SetReflectionTypeName(const String& typeName)
{
    reflectionTypeName = typeName;
}

void UIFlowControllerComponent::SetLuaScriptPath(const FilePath& filePath)
{
    luaScriptPath = filePath;
}
}