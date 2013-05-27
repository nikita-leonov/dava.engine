/*==================================================================================
    Copyright (c) 2008, DAVA, INC
    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of the DAVA, INC nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE DAVA, INC AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DAVA, INC BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/

#include "UIAggregatorControl.h"
#include "FileSystem/FileSystem.h"

using namespace DAVA;

#define AGGREGATOR_PATH "aggregatorPath"

REGISTER_CLASS(UIAggregatorControl);

UIAggregatorControl::UIAggregatorControl(const Rect &rect, bool rectInAbsoluteCoordinates) :
	UIControl(rect, rectInAbsoluteCoordinates)
{
	
}


UIControl* UIAggregatorControl::Clone()
{
	UIAggregatorControl* c = new UIAggregatorControl(Rect(relativePosition.x, relativePosition.y, size.x, size.y));
	c->CopyDataFrom(this);
	return c;
}

YamlNode* UIAggregatorControl::SaveToYamlNode(UIYamlLoader * loader)
{
	YamlNode* node = UIControl::SaveToYamlNode(loader);
	SetPreferredNodeType(node, "UIAggregatorControl");
	node->Set(AGGREGATOR_PATH, aggregatorPath.GetAbsolutePathname());
	return node;
}

void UIAggregatorControl::LoadFromYamlNode(YamlNode * node, UIYamlLoader * loader)
{
	UIControl::LoadFromYamlNode(node, loader);
	
	YamlNode * pathNode = node->Get(AGGREGATOR_PATH);
	if (pathNode)
	{
		aggregatorPath = FilePath(pathNode->AsString());
		String aggregatorFileName = aggregatorPath.GetFilename();

		aggregatorPath = loader->GetCurrentPath() + aggregatorFileName;

		UIYamlLoader loader;
		loader.Load(this, aggregatorPath);

		aggregatorPath = FilePath(aggregatorFileName);
	}
}

List<UIControl* >& UIAggregatorControl::GetRealChildren()
{
	realChilds = UIControl::GetRealChildren();
	
	for (List<UIControl*>::iterator iter = aggregatorControls.begin(); iter != aggregatorControls.end(); ++iter)
	{
		UIControl* control = (*iter);
		realChilds.remove(control);
	}
	return realChilds;
}

void UIAggregatorControl::AddAggregatorChild(UIControl* uiControl)
{
	//AddControl(uiControl);
	//BringChildFront(uiControl);
	aggregatorControls.push_back(uiControl);
}

void UIAggregatorControl::SetAggregatorPath(const FilePath& path)
{
	aggregatorPath = path;
}

const FilePath & UIAggregatorControl::GetAggregatorPath() const
{
	return aggregatorPath;
}