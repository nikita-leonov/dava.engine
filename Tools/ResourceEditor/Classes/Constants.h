#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

namespace ResourceEditor 
{

enum eNodeType
{
    NODE_LANDSCAPE  = 0,
    NODE_LIGHT,
    NODE_SERVICE_NODE,
    NODE_BOX,
    NODE_SPHERE,
    NODE_CAMERA,
    NODE_IMPOSTER,
    NODE_PARTICLE_EMITTER,
    NODE_USER_NODE,
	NODE_SWITCH_NODE,
	NODE_PARTICLE_EFFECT,
    
    NODE_COUNT
};
    
enum eViewportType
{
    VIEWPORT_IPHONE = 0,
    VIEWPORT_RETINA,
    VIEWPORT_IPAD,
    VIEWPORT_DEFAULT,
    
    VIEWPORT_COUNT
};
  
  
enum eHideableWidgets
{
    HIDABLEWIDGET_SCENE_GRAPH = 0,
    HIDABLEWIDGET_DATA_GRAPH,
    HIDABLEWIDGET_ENTITY_GRAPH,
    HIDABLEWIDGET_PROPERTIES,
    HIDABLEWIDGET_LIBRARY,
	HIDABLEWIDGET_REFERENCES,
    
    HIDABLEWIDGET_TOOLBAR,
    
    HIDABLEWIDGET_CUSTOMCOLORS,
	HIDEBLEWIDGET_VISIBILITYCHECKTOOL,
    
    HIDABLEWIDGET_COUNT
};
    
    
};




#endif //#ifndef __CONSTANTS_H__