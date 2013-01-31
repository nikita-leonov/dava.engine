/*==================================================================================
    Copyright (c) 2012, DAVA Consulting, LLC
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the DAVA Consulting, LLC nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE DAVA CONSULTING, LLC AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL DAVA CONSULTING, LLC BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    Revision History:
        * Created by Yury Danilov 
=====================================================================================*/
#include "Scene3D/UserNode.h"
#include "Render/RenderHelper.h"
#include "Render/RenderManager.h"

namespace DAVA
{
REGISTER_CLASS(UserNode);
	
UserNode::UserNode()
	:drawBox(Vector3(-0.5f, -0.5f, -0.5f), Vector3(0.5f, 0.5f, 0.5f))
{
//	SetDebugFlags(GetDebugFlags() | DEBUG_DRAW_USERNODE);
}

UserNode::~UserNode()
{

}

void UserNode::Draw()
{    
	SceneNode::Draw();
	if (!(flags & NODE_VISIBLE) || !(flags & NODE_UPDATABLE) || (flags & NODE_INVALID))return;

}



AABBox3 UserNode::GetWTMaximumBoundingBox()
{
	AABBox3 retBBox = drawBox;
	drawBox.GetTransformedBox(GetWorldTransform(), retBBox);
	return retBBox;
}


SceneNode* UserNode::Clone(SceneNode *dstNode)
{
	if (!dstNode) 
	{
		DVASSERT_MSG(IsPointerToExactClass<UserNode>(this), "Can clone only UserNode");
		dstNode = new UserNode();
	}
	SceneNode::Clone(dstNode);
	return dstNode;
}


};