#include "LightPropertyControl.h"


LightPropertyControl::LightPropertyControl(const Rect & rect, bool createNodeProperties)
:	NodesPropertyControl(rect, createNodeProperties)
{
	types.push_back("Directional");
	types.push_back("Spot");
	types.push_back("Point");
	types.push_back("Sky");
}

LightPropertyControl::~LightPropertyControl()
{

}

void LightPropertyControl::ReadFrom(SceneNode * sceneNode)
{
	NodesPropertyControl::ReadFrom(sceneNode);

	LightNode * light = dynamic_cast<LightNode *>(sceneNode);
	DVASSERT(light);

	propertyList->SetBoolPropertyValue("Used in static lighting", sceneNode->GetCustomProperties()->GetBool("editor.staticlight.used", false));

    propertyList->AddSection("Light", GetHeaderState("Light", true));
        
    propertyList->AddComboProperty("Type", types);
	propertyList->SetComboPropertyIndex("Type", light->GetType());

    propertyList->AddFloatProperty("r");
	propertyList->SetFloatPropertyValue("r", light->GetColor().r);

    propertyList->AddFloatProperty("g");
	propertyList->SetFloatPropertyValue("g", light->GetColor().g);

    propertyList->AddFloatProperty("b"); 
	propertyList->SetFloatPropertyValue("b", light->GetColor().b);

	propertyList->AddSection("Static light", GetHeaderState("Static light", true));

	propertyList->AddBoolProperty("Enable");
	propertyList->SetBoolPropertyValue("Enable", light->GetCustomProperties()->GetBool("editor.staticlight.enable", true));

	propertyList->AddBoolProperty("Cast shadows");
	propertyList->SetBoolPropertyValue("Cast shadows", light->GetCustomProperties()->GetBool("editor.staticlight.castshadows", true));

	propertyList->AddFloatProperty("Intensity");
	propertyList->SetFloatPropertyValue("Intensity", light->GetCustomProperties()->GetFloat("editor.intensity", 1.f));

	if(LightNode::TYPE_DIRECTIONAL == light->GetType())
	{
		propertyList->AddFloatProperty("Shadow angle");
		propertyList->SetFloatPropertyValue("Shadow angle", light->GetCustomProperties()->GetFloat("editor.shadowangle", 0.f));

		propertyList->AddIntProperty("Shadow samples");
		propertyList->SetIntPropertyValue("Shadow samples", light->GetCustomProperties()->GetInt32("editor.shadowsamples", 1));
	}
}

void LightPropertyControl::WriteTo(SceneNode * sceneNode)
{
	NodesPropertyControl::WriteTo(sceneNode);

	LightNode *light = dynamic_cast<LightNode *>(sceneNode);
	DVASSERT(light);

	Color color(
		propertyList->GetFloatPropertyValue("r"),
		propertyList->GetFloatPropertyValue("g"),
		propertyList->GetFloatPropertyValue("b"),
		1.f);
	light->SetColor(color);

	int32 type = propertyList->GetComboPropertyIndex("Type");
	light->SetType((LightNode::eType)type);

	bool enable = propertyList->GetBoolPropertyValue("Enable");
	light->GetCustomProperties()->SetBool("editor.staticlight.enable", enable);

	bool castShadows = propertyList->GetBoolPropertyValue("Cast shadows");
	light->GetCustomProperties()->SetBool("editor.staticlight.castshadows", castShadows);

	float32 intensity = propertyList->GetFloatPropertyValue("Intensity");
	light->GetCustomProperties()->SetFloat("editor.intensity", intensity);

	if(LightNode::TYPE_DIRECTIONAL == light->GetType())
	{
		float32 shadowAngle = propertyList->GetFloatPropertyValue("Shadow angle");
		light->GetCustomProperties()->SetFloat("editor.shadowangle", shadowAngle);

		int32 shadowSamples = propertyList->GetIntPropertyValue("Shadow samples");
		light->GetCustomProperties()->SetInt32("editor.shadowsamples", shadowSamples);
	}
}

void LightPropertyControl::OnComboIndexChanged(
                                    PropertyList *forList, const String &forKey, int32 newItemIndex, const String &newItemKey)
{
    if("Type" == forKey)
    {
        LightNode *light = dynamic_cast<LightNode *>(currentNode);
        light->SetType((LightNode::eType)newItemIndex);
    }

    NodesPropertyControl::OnComboIndexChanged(forList, forKey, newItemIndex, newItemKey);
}

void LightPropertyControl::OnBoolPropertyChanged(PropertyList *forList, const String &forKey, bool newValue)
{
    if("Enable" == forKey)
    {
        LightNode *light = dynamic_cast<LightNode *>(currentNode);
        light->GetCustomProperties()->SetBool("editor.staticlight.enable", newValue);
    }
    else if("Cast shadows" == forKey)
    {
        LightNode *light = dynamic_cast<LightNode *>(currentNode);
        light->GetCustomProperties()->SetBool("editor.staticlight.castshadows", newValue);
    }

    NodesPropertyControl::OnBoolPropertyChanged(forList, forKey, newValue);
}

void LightPropertyControl::OnFloatPropertyChanged(PropertyList *forList, const String &forKey, float newValue)
{
    if("r" == forKey || "g" == forKey || "b" == forKey)
    {
        LightNode *light = dynamic_cast<LightNode *>(currentNode);
        Color color(
                    propertyList->GetFloatPropertyValue("r"),
                    propertyList->GetFloatPropertyValue("g"),
                    propertyList->GetFloatPropertyValue("b"),
                    1.f);
        light->SetColor(color);
    }
    else if("Intensity" == forKey)
    {
        LightNode *light = dynamic_cast<LightNode *>(currentNode);
        light->GetCustomProperties()->SetFloat("editor.intensity", newValue);
    }

    NodesPropertyControl::OnFloatPropertyChanged(forList, forKey, newValue);
}

