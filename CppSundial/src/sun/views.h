#pragma once
#include "sun/sunUtils.h"
#include "network/network.h"



// NOTE: the info structs are defined in here because of some very annoying header file problems.
// please do not ask questions

struct sun_ControlsInfo {
    // NOTE: socket connection is reactive to these, no updates need to happen in user code
    bool usingSockets = true;
    bool sim = true;

    BumpAlloc* replayArena = nullptr;
    net_Table replayTable;
    // signals to copy samples from replay table to main table at current time
    bool refreshTableFlag = false;
};
void sun_controlsBuild(sun_ControlsInfo* info);


struct sun_FieldInfo {
    Transform camTransform;
    Transform camTarget = { 0, 6, 0, -90, 0, 0, 1, 1, 1 };
};
void sun_fieldBuild(sun_FieldInfo* info, gfx_Framebuffer* fb);


// CLEANUP: prefix these?
#define GRAPH2D_VCOUNT 700
#define GRAPH2D_LINECOUNT 6
#define GRAPH2D_SAMPLE_WINDOW 5.0f
struct sun_Graph2dInfo {
    NTKey keys[GRAPH2D_LINECOUNT] = { 0 };
    V4f colors[GRAPH2D_LINECOUNT];

    float top = 1.2;
    float bottom = -1.2;

    // allocated and removed per instance
    gfx_SSBO* lineVerts[GRAPH2D_LINECOUNT] = { 0 };
    gfx_SSBO* gridVerts;
    gfx_SSBO* timeVerts;
};

void sun_graph2dInit(sun_Graph2dInfo* info);
void sun_graph2dDeinit(sun_Graph2dInfo* info);
void sun_graph2dBuild(sun_Graph2dInfo* info, gfx_Framebuffer* target);


struct sun_NetInfo {
    float clipPos = 0;
};
void sun_networkBuild(sun_NetInfo* info);


struct sun_PathInfo {
    BumpAlloc resArena;
    gfx_SSBO* pathSSBO = nullptr;
    gfx_SSBO* connectSSBO = nullptr;
    V2f camPos = { 0, 0 };
    float camHeight = 4;

    bool pathDirty = true;
    V2f* path = nullptr;
    U32 pathPtCount = 0; // number of points, including tangents and anchors
};

#define PATH_BEZIERSUBDIVCOUNT 40
void sun_pathsInit(sun_PathInfo* info);
void sun_pathsDeinit(sun_PathInfo* info);
void sun_pathsBuild(sun_PathInfo* info, gfx_Framebuffer* fb);


// TODO: add bool boxes
#define POWER_INDICATOR_COUNT 30
struct sun_PowerIndicatorInfo {
    NTKey keys[POWER_INDICATOR_COUNT];
    float scrollPosition = 0;
};
void sun_powerIndicatorsBuild(sun_PowerIndicatorInfo* info);


struct sun_SwerveInfo {
};
void sun_swerveBuild(sun_SwerveInfo* info, gfx_Framebuffer* target);







enum ViewType {
    viewType_graph2d,
    viewType_field,
    viewType_net,
    viewType_swerveDrive,
    viewType_powerIndicators,
    viewType_controls,
    viewType_paths,
};

union ViewUnion {
    sun_Graph2dInfo graph2dInfo;
    sun_FieldInfo fieldInfo;
    sun_NetInfo netInfo;
    sun_SwerveInfo swerveInfo;
    sun_PowerIndicatorInfo powerIndicatorInfo;
    sun_PathInfo pathInfo;

    ViewUnion() {  };
};

struct View {
    ViewUnion data;
    ViewType type;
    gfx_Framebuffer* target; // CLEANUP: bad
    BumpAlloc* res;
};

void initView(View* v);
void updateView(View* v);
void makeViewSrc(const char* name, gfx_Texture* texture, ViewType type);
void makeView(const char* name, View* v);

#ifdef SUN_IMPL
#include "colors.h"
#include "base/utils.h"
#include "base/str.h"
#include "graphics.h"
#include "network/network.h"
#include "sun/sun.h"
#include "sun/sunUtils.h"

extern SunGlobs globs;

void initView(View* v) {
    if(v->type == viewType_field) { v->data.fieldInfo = sun_FieldInfo(); }
    else if(v->type == viewType_graph2d) { sun_graph2dInit(&v->data.graph2dInfo); }
    else if(v->type == viewType_swerveDrive) { v->data.swerveInfo = sun_SwerveInfo(); }
    else if(v->type == viewType_net) { v->data.netInfo = sun_NetInfo(); }
    else if(v->type == viewType_powerIndicators) { v->data.powerIndicatorInfo = sun_PowerIndicatorInfo(); }
    else if(v->type == viewType_controls) { }
    else if(v->type == viewType_paths) { sun_pathsInit(&v->data.pathInfo); }
    else { ASSERT(false); };
}

void updateView(View* v) {
    if(v->type == viewType_field) { sun_fieldBuild(&v->data.fieldInfo, v->target); }
    else if(v->type == viewType_graph2d) { sun_graph2dBuild(&v->data.graph2dInfo, v->target); }
    else if(v->type == viewType_swerveDrive) { sun_swerveBuild(&v->data.swerveInfo, v->target); }
    else if(v->type == viewType_net) { sun_networkBuild(&v->data.netInfo); }
    else if(v->type == viewType_powerIndicators) { sun_powerIndicatorsBuild(&v->data.powerIndicatorInfo); }
    else if(v->type == viewType_controls) { sun_controlsBuild(&globs.ctrlInfo); }
    else if(v->type == viewType_paths) { sun_pathsBuild(&v->data.pathInfo, v->target); }
    else { ASSERT(false); };
}

// TODO: button texures
void makeViewSrc(const char* name, gfx_Texture* texture, ViewType type) {

    blu_Area* a = blu_areaMake(name,
        blu_areaFlags_HOVER_ANIM | blu_areaFlags_DRAW_TEXTURE | blu_areaFlags_CLICKABLE | blu_areaFlags_DRAW_BACKGROUND);
    a->style.backgroundColor = v4f_lerp(col_darkGray, col_lightGray, a->target_hoverAnim);
    a->texture = texture;

    a->dropType = dropMasks_VIEW_TYPE;
    a->dropVal = (void*)type;
    if(blu_interactionFromWidget(a).held) {
        blu_parentScope(blu_getCursorParent()) {
            blu_styleScope(blu_Style()) {
            blu_style_style(&globs.borderStyle);
            blu_style_sizeX({ blu_sizeKind_TEXT, 0 });
            blu_style_sizeY({ blu_sizeKind_TEXT, 0 });
            blu_style_borderSize(2);
                a = blu_areaMake("drag indicator", blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DRAW_TEXT);
                blu_areaAddDisplayStr(a, name);
            }
        }
    }
}

void makeView(const char* name, View* v) {

    blu_Area* a = blu_areaMake(name, blu_areaFlags_DROP_EVENTS | blu_areaFlags_HOVER_ANIM);
    blu_style_childLayoutAxis(blu_axis_Y, &a->style);
    a->dropTypeMask = dropMasks_VIEW_TYPE;

    blu_WidgetInteraction inter = blu_interactionFromWidget(a);
    if(inter.dropped) {

        // CLEANUP: ew
        if(v->type == viewType_graph2d) {
            sun_graph2dDeinit(&v->data.graph2dInfo);
        }
        else if(v->type == viewType_paths) {
            sun_pathsDeinit(&v->data.pathInfo);
        }

        v->type = (ViewType)(U64)inter.dropVal; // sorry
        initView(v);
    }

    float t = a->target_hoverAnim;

    blu_parentScope(a) {
        blu_styleScope(blu_Style()) {
        blu_style_sizeX({ blu_sizeKind_PERCENT, 1 });
        blu_style_sizeY({ blu_sizeKind_PERCENT, 1 });

            updateView(v);

            a = blu_areaMake("hoverIndicator", blu_areaFlags_FLOATING | blu_areaFlags_DRAW_BACKGROUND);
            a->style.backgroundColor = v4f_lerp(V4f(0, 0, 0, 0), V4f(1, 1, 1, 0.3), t);
        }
    }
}

#endif
