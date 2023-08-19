#pragma once

#include "GLAD/gl.h"
#include "GLFW/glfw3.h"
#include "stb_image/stb_image.h"
#include <cstring>

#include "colors.h"
#include "base/arr.h"
#include "graphics.h"
#include "base/utils.h"
#include "blue/blue.h"
#include "network/network.h"
#include "network/sockets.h"

// TODO: angle visualizer
// TODO: vector visualizer

// contains a string and a bunch of chars
struct NTKey {
    str str = { nullptr, 0 };
    U8 chars[255] = { 0 };
};
struct LineVert {
    V4f pos;
    V4f color;
};



#define GRAPH2D_VCOUNT 700
#define GRAPH2D_LINECOUNT 6
#define GRAPH2D_SAMPLE_WINDOW 5.0f
struct Graph2dInfo {
    NTKey keys[GRAPH2D_LINECOUNT] = { 0 };
    V4f colors[GRAPH2D_LINECOUNT];

    float top = 0.9;
    float bottom = -0.9;

    // allocated and removed per instance
    gfx_SSBO* lineVerts[GRAPH2D_LINECOUNT] = { 0 };
    gfx_SSBO* gridVerts;
    gfx_SSBO* timeVerts;
};
void draw_graph2d(Graph2dInfo* info, gfx_Framebuffer* target);
void initGraph2dInfo(Graph2dInfo* info) {

    info->colors[0] = col_green;
    info->keys[0].str = str_copy(STR("FLSteerSpeed"), info->keys[0].chars);

    info->colors[1] = col_red;
    info->keys[1].str = str_copy(STR("FLSteerPos"), info->keys[1].chars);

    info->colors[2] = col_purple;
    info->colors[3] = col_yellow;
    info->colors[4] = col_black;
    info->colors[5] = col_white;

    for(int i = 0; i < GRAPH2D_LINECOUNT; i++) {
        info->lineVerts[i] = gfx_registerSSBO(nullptr, 0, true);
    }
    info->gridVerts = gfx_registerSSBO(nullptr, 0, true);
    info->timeVerts = gfx_registerSSBO(nullptr, 0, true);
}
void deinitGraph2dInfo(Graph2dInfo* info) {
    for(int i = 0; i < GRAPH2D_LINECOUNT; i++) {
        gfx_freeSSBO(info->lineVerts[i]);
    }
    gfx_freeSSBO(info->gridVerts);
    gfx_freeSSBO(info->timeVerts);
}

struct FieldInfo {
    Transform camTransform;
    Transform camTarget = { 0, 6, 0, -90, 0, 0, 1, 1, 1 };
};
void draw_field(FieldInfo* info, gfx_Framebuffer* fb);

struct NetInfo {
    float clipPos = 0;
};
void draw_network(NetInfo* info);

struct SwerveDriveInfo {

};
void draw_swerveDrive(SwerveDriveInfo* info, gfx_Framebuffer* target);

// TODO: add bool boxes
#define POWER_INDICATOR_COUNT 30
struct PowerIndicatorInfo {
    NTKey keys[POWER_INDICATOR_COUNT];
    float scrollPosition = 0;
};

void draw_powerIndicators(PowerIndicatorInfo* info);





struct ControlsInfo {
    // NOTE: socket connection is reactive to these, no updates need to happen in user code
    bool usingSockets = true;
    bool sim = true;

    BumpAlloc* replayArena = nullptr;
    net_Table replayTable;
    // signals to copy samples from replay table to main table at current time
    bool refreshTableFlag = false;
};
void draw_controls(ControlsInfo* info);



struct PathInfo {
    BumpAlloc resArena;
    gfx_SSBO* pathSSBO = nullptr;
    V2f camPos = { 0, 0 };
    float camHeight = 4;

    bool pathDirty = true;
    V2f* path = nullptr;
    U32 pathPtCount = 0; // number of points, including tangents and anchors
};
#define PATH_BEZIERSUBDIVCOUNT 40
void init_paths(PathInfo* info) {
    *info = PathInfo();
    bump_allocate(&info->resArena, 10000);
    info->pathSSBO = gfx_registerSSBO(nullptr, 0, false);

    info->pathPtCount = 8;
    V2f* p = BUMP_PUSH_ARR(&info->resArena, 8, V2f);
    p[0] = V2f(0, 0);
    p[1] = V2f(1, 1);
    p[2] = V2f(3, 1);
    p[3] = V2f(3, 0);

    p[4] = V2f(3, 0);
    p[5] = V2f(3, -1);
    p[6] = V2f(1, -1);
    p[7] = V2f(0, 1);
    info->path = p;
}
void draw_paths(PathInfo* info, gfx_Framebuffer* fb);
void deinit_paths(PathInfo* info) {
    bump_free(&info->resArena);
    gfx_freeSSBO(info->pathSSBO);
}


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
    Graph2dInfo graph2dInfo;
    FieldInfo fieldInfo;
    NetInfo netInfo;
    SwerveDriveInfo swerveDriveInfo;
    PowerIndicatorInfo powerIndicatorInfo;
    PathInfo pathInfo;

    ViewUnion() {  };
};
struct View {
    ViewUnion data;
    ViewType type;
    gfx_Framebuffer* target; // CLEANUP: bad
    BumpAlloc* res;
};



enum DropMasks {
    dropMasks_NONE = 0,
    dropMasks_NT_PROP = (1 << 0),
    dropMasks_VIEW_TYPE = (1 << 1),
};



// TEMP:
// adds a framebuffer to an area, and resizes based on area size
// doesnt rezise if area is <= 0 on either axis
void areaAddFB(blu_Area* area, gfx_Framebuffer* target) {

    area->flags |= blu_areaFlags_DRAW_TEXTURE;
    area->texture = target->texture;

    int w = (int)area->calculatedSizes[blu_axis_X];
    int h = (int)area->calculatedSizes[blu_axis_Y];
    if(w != target->texture->width || h != target->texture->height) {
        if(w > 0 && h > 0) {
            gfx_resizeFramebuffer(target, w, h);
        }
    }
}


#define UI_SCROLL_SENSITIVITY 40
#define UI_SCROLL_BAR_SIZE 10

// returns parent to clip area that elems can be added
// NOTE: any styling cant be applied in post because it returns a child area, so put sizing in a scope
blu_Area* makeScrollArea(float* pos) {

    blu_Area* clip = nullptr;

    blu_Area* a = blu_areaMake("scrollParent", blu_areaFlags_DRAW_BACKGROUND);
    blu_style_childLayoutAxis(blu_axis_X, &a->style);
    float clipSize = a->calculatedSizes[blu_axis_Y];
    float clipMax;

    blu_parentScope(a) {

        a = blu_areaMake("clip", blu_areaFlags_VIEW_OFFSET | blu_areaFlags_CLICKABLE | blu_areaFlags_SCROLLABLE );
        clip = a;
        blu_style_sizeX({ blu_sizeKind_REMAINDER, 0 }, &a->style);
        blu_style_sizeY({ blu_sizeKind_CHILDSUM, 0 }, &a->style);
        blu_style_childLayoutAxis(blu_axis_Y, &a->style);
        *pos += blu_interactionFromWidget(a).scrollDelta * UI_SCROLL_SENSITIVITY;
        clipMax = a->calculatedSizes[blu_axis_Y];




        blu_Area* spacer = nullptr;
        if(clipMax > clipSize) {
            a = blu_areaMake("scrollArea", blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_SCROLLABLE);
            a->style.backgroundColor = col_darkGray;
            a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, UI_SCROLL_BAR_SIZE };
            a->style.sizes[blu_axis_Y] = { blu_sizeKind_PERCENT, 1 };
            a->style.childLayoutAxis = blu_axis_Y;
            *pos += blu_interactionFromWidget(a).scrollDelta * UI_SCROLL_SENSITIVITY;

            blu_parentScope(a) {

                a = blu_areaMake("spacer", 0);
                spacer = a;

                a = blu_areaMake("bar", blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_CLICKABLE);
                a->style.backgroundColor = col_lightGray;
                a->style.sizes[blu_axis_X] = { blu_sizeKind_PERCENT, 1 };
                a->style.sizes[blu_axis_Y] = { blu_sizeKind_PERCENT, clipSize / clipMax };
                *pos += blu_interactionFromWidget(a).dragDelta.y / clipSize * clipMax;
            }

            *pos = max(*pos, 0);
            *pos = min(*pos, clipMax - clipSize);
            spacer->style.sizes[blu_axis_Y] = { blu_sizeKind_PX, ((*pos) / clipMax) * clipSize };
        } else {
            *pos = 0;
        }
    }

    clip->viewOffset = { 0, *pos };

    return clip;
}

blu_WidgetInteraction makeButton(str text, V4f backColor, V4f hoverColor) {
    // TODO: button textures
    blu_Area* a = blu_areaMake(text, blu_areaFlags_CLICKABLE | blu_areaFlags_CENTER_TEXT | blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DRAW_TEXT | blu_areaFlags_HOVER_ANIM);
    a->style.backgroundColor = v4f_lerp(backColor, hoverColor, a->target_hoverAnim);
    blu_areaAddDisplayStr(a, text);
    return blu_interactionFromWidget(a);
}
blu_WidgetInteraction makeButton(str text, V4f hoverColor) {
    blu_Area* a = blu_areaMake(text, blu_areaFlags_CLICKABLE | blu_areaFlags_CENTER_TEXT | blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DRAW_TEXT | blu_areaFlags_HOVER_ANIM);
    a->style.backgroundColor = v4f_lerp(a->style.backgroundColor, hoverColor, a->target_hoverAnim);
    blu_areaAddDisplayStr(a, text);
    return blu_interactionFromWidget(a);
}


#define UI_VIEW_COUNT 4
static struct UIGlobs {

    gfx_Shader* sceneShader3d = nullptr;
    gfx_Shader* sceneShader2d = nullptr;
    gfx_Shader* lineShader = nullptr;

    float rightSize;
    float downSizeL;
    float downSizeR;

    View views[UI_VIEW_COUNT];

    gfx_Texture* solidTex;
    gfx_Texture* wheelTex = nullptr;
    gfx_Texture* treadTex = nullptr;
    gfx_Texture* arrowTex = nullptr;
    gfx_Texture* fieldTex = nullptr;

    gfx_VertexArray* robotVA = nullptr;
    gfx_IndexBuffer* robotIB = nullptr;
    gfx_VertexArray* fieldVA = nullptr;
    gfx_IndexBuffer* fieldIB = nullptr;
    gfx_VertexArray* quadVA = nullptr;
    gfx_IndexBuffer* quadIB = nullptr;

    gfx_VertexArray* emptyVA = nullptr;

    ControlsInfo ctrlInfo;

    net_Table table;
    GLFWwindow* window;
    float dt;
    BumpAlloc* scratch;
    float curTime;
} globs;

// Keeping keys globally allocated for now
/*

void NTKeyFree(NTKey* k) {
    k->nextFree = globs.firstFreeNTKey;
    globs.firstFreeNTKey = k;
}
NTKey* NTKeyAlloc(BumpAlloc* arena) {
    NTKey* k = globs.firstFreeNTKey;
    if(!k) { k = BUMP_PUSH_NEW(arena, NTKey); }
    else {
        globs.firstFreeNTKey = k->nextFree;
        *k = NTKey();
    }
    return k;
}
*/

void initView(View* v) {
    if(v->type == viewType_field) { v->data.fieldInfo = FieldInfo(); }
    else if(v->type == viewType_graph2d) { v->data.graph2dInfo = Graph2dInfo(); initGraph2dInfo(&v->data.graph2dInfo); }
    else if(v->type == viewType_swerveDrive) { v->data.swerveDriveInfo = SwerveDriveInfo(); }
    else if(v->type == viewType_net) { v->data.netInfo = NetInfo(); }
    else if(v->type == viewType_powerIndicators) { v->data.powerIndicatorInfo = PowerIndicatorInfo(); }
    else if(v->type == viewType_controls) { }
    else if(v->type == viewType_paths) { init_paths(&v->data.pathInfo); }
    else { ASSERT(false); };
}

void updateView(View* v) {
    if(v->type == viewType_field) { draw_field(&v->data.fieldInfo, v->target); }
    else if(v->type == viewType_graph2d) { draw_graph2d(&v->data.graph2dInfo, v->target); }
    else if(v->type == viewType_swerveDrive) { draw_swerveDrive(&v->data.swerveDriveInfo, v->target); }
    else if(v->type == viewType_net) { draw_network(&v->data.netInfo); }
    else if(v->type == viewType_powerIndicators) { draw_powerIndicators(&v->data.powerIndicatorInfo); }
    else if(v->type == viewType_controls) { draw_controls(&globs.ctrlInfo); }
    else if(v->type == viewType_paths) { draw_paths(&v->data.pathInfo, v->target); }
    else { ASSERT(false); };
}






blu_Style borderStyle;


void ui_init(BumpAlloc* frameArena, BumpAlloc* replayArena, gfx_Texture* solidTex) {

    globs.rightSize = 400;
    globs.downSizeL = 100;
    globs.downSizeR = 400;

    globs.solidTex = solidTex;

    int w, h, bpp;
    stbi_set_flip_vertically_on_load(1);
    U8* data;

    blu_style_borderColor(col_black, &borderStyle);
    blu_style_borderSize(1, &borderStyle);

    for(int i = 0; i < UI_VIEW_COUNT; i++) {
        View* v = &(globs.views[i]);
        v->target = gfx_registerFramebuffer();
    }
    globs.views[0].type = viewType_field;
    globs.views[1].type = viewType_controls;
    globs.views[2].type = viewType_graph2d;
    globs.views[3].type = viewType_net;

    initView(&globs.views[0]);
    initView(&globs.views[1]);
    initView(&globs.views[2]);
    initView(&globs.views[3]);

    globs.ctrlInfo = ControlsInfo();
    globs.ctrlInfo.replayArena = replayArena;

    bool res = gfx_loadOBJMesh("res/models/Chassis3.obj", frameArena, &globs.robotVA, &globs.robotIB);
    ASSERT(res);
    bump_clear(frameArena);

    res = gfx_loadOBJMesh("res/models/plane.obj", frameArena, &globs.fieldVA, &globs.fieldIB);
    ASSERT(res);
    bump_clear(frameArena);

    data = stbi_load("res/textures/field.png", &w, &h, &bpp, 4);
    ASSERT(data);
    globs.fieldTex = gfx_registerTexture(data, w, h, gfx_texPxType_RGBA8);

    data = stbi_load("res/textures/swerveWheel.png", &w, &h, &bpp, 4);
    ASSERT(data);
    globs.wheelTex = gfx_registerTexture(data, w, h, gfx_texPxType_RGBA8);

    data = stbi_load("res/textures/swerveTread.png", &w, &h, &bpp, 4);
    ASSERT(data);
    globs.treadTex = gfx_registerTexture(data, w, h, gfx_texPxType_RGBA8);

    data = stbi_load("res/textures/vector.png", &w, &h, &bpp, 4);
    ASSERT(data);
    globs.arrowTex = gfx_registerTexture(data, w, h, gfx_texPxType_RGBA8);


    U32 ibData[] = { 0, 1, 2,   2, 3, 0 };
    globs.quadIB = gfx_registerIndexBuffer(ibData, sizeof(ibData) / sizeof(U32), false);
    F32 vbData[] = { 0, 0, 0, 0,   0, 1, 0, 1,   1, 1, 1, 1,   1, 0, 1, 0 };
    globs.quadVA = gfx_registerVertexArray(gfx_vtype_POS2F_UV, vbData, sizeof(vbData), false);


    globs.emptyVA = gfx_registerVertexArray(gfx_vtype_NONE, nullptr, 0, false);


    globs.sceneShader2d = gfx_registerShader(gfx_vtype_POS2F_UV, "res/shaders/2d.vert", "res/shaders/2d.frag", frameArena);
    globs.sceneShader2d->passUniformBindFunc = [](gfx_Pass* pass, gfx_UniformBlock* uniforms) {
        int loc = glGetUniformLocation(pass->shader->id, "uVP");
        glUniformMatrix4fv(loc, 1, false, &(uniforms->vp)[0]);

        gfx_bindVertexArray(pass, globs.quadVA);
        gfx_bindIndexBuffer(pass, globs.quadIB);
    };
    globs.sceneShader2d->uniformBindFunc = [](gfx_Pass* pass, gfx_UniformBlock* uniforms) {

        int loc = glGetUniformLocation(pass->shader->id, "uModel");
        glUniformMatrix4fv(loc, 1, false, &(uniforms->model)[0]);

        loc = glGetUniformLocation(pass->shader->id, "uSrcStart");
        glUniform2f(loc, uniforms->srcStart.x, uniforms->srcStart.y);
        loc = glGetUniformLocation(pass->shader->id, "uSrcEnd");
        glUniform2f(loc, uniforms->srcEnd.x, uniforms->srcEnd.y);

        loc = glGetUniformLocation(pass->shader->id, "uColor");
        glUniform4f(loc, uniforms->color.x, uniforms->color.y, uniforms->color.z, uniforms->color.w);

        loc = glGetUniformLocation(pass->shader->id, "uTexture");
        glUniform1i(loc, 0);
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, uniforms->texture->id);

        uniforms->vertCount = globs.quadIB->count;
    };


    globs.sceneShader3d = gfx_registerShader(gfx_vtype_POS3F_UV, "res/shaders/3d.vert", "res/shaders/3d.frag", frameArena);
    globs.sceneShader3d->passUniformBindFunc = [](gfx_Pass* pass, gfx_UniformBlock* uniforms) {
        int loc = glGetUniformLocation(pass->shader->id, "uVP");
        glUniformMatrix4fv(loc, 1, false, &(uniforms->vp)[0]);
    };
    globs.sceneShader3d->uniformBindFunc = [](gfx_Pass* pass, gfx_UniformBlock* uniforms) {
        int loc = glGetUniformLocation(pass->shader->id, "uColor");
        glUniform4f(loc, uniforms->color.x, uniforms->color.y, uniforms->color.z, uniforms->color.w);

        loc = glGetUniformLocation(pass->shader->id, "uModel");
        glUniformMatrix4fv(loc, 1, false, &(uniforms->model)[0]);

        loc = glGetUniformLocation(pass->shader->id, "uTexture");
        glUniform1i(loc, 0);
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, uniforms->texture->id);

        gfx_bindVertexArray(pass, uniforms->va);
        gfx_bindIndexBuffer(pass, uniforms->ib);
        uniforms->vertCount = uniforms->ib->count;
    };


    globs.lineShader = gfx_registerShader(gfx_vtype_NONE, "res/shaders/line.vert", "res/shaders/line.frag", frameArena);
    globs.lineShader->passUniformBindFunc = [](gfx_Pass* pass, gfx_UniformBlock* uniforms) {
        int loc = glGetUniformLocation(pass->shader->id, "uVP");
        glUniformMatrix4fv(loc, 1, false, &(uniforms->vp)[0]);

        loc = glGetUniformLocation(pass->shader->id, "uResolution");
        glUniform2f(loc, uniforms->resolution.x, uniforms->resolution.y);
    };
    globs.lineShader->uniformBindFunc = [](gfx_Pass* pass, gfx_UniformBlock* uniforms) {
        int loc = glGetUniformLocation(pass->shader->id, "uThickness");
        glUniform1f(loc, uniforms->thickness);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, uniforms->ssbo->id);
        gfx_bindVertexArray(pass, globs.emptyVA);
    };
}



// ptCount is inclusive of tangent points, curve interpreted as a cubic bezier
V2f bezierSample(V2f* pts, U32 ptCount, float t) {
    U32 segCount = ptCount / 4;
    U32 segIdx = (U32)(t*segCount);
    if(segIdx >= segCount) {
        return pts[ptCount-1];
    }

    float pct = fmodf(t*segCount, 1);
    V2f* p = &pts[segIdx*4];
    V2f a = v2f_lerp(p[0], p[1], pct);
    V2f b = v2f_lerp(p[1], p[2], pct);
    V2f c = v2f_lerp(p[2], p[3], pct);

    V2f d = v2f_lerp(a, b, pct);
    V2f e = v2f_lerp(b, c, pct);
    return v2f_lerp(d, e, pct);
}

void draw_paths(PathInfo* info, gfx_Framebuffer* fb) {
    blu_Area* a = blu_areaMake("paths", 0);
    areaAddFB(a, fb);
    float width = fb->texture->width;
    float height = fb->texture->height;



    V2f move = {
        (float)glfwGetKey(globs.window, GLFW_KEY_D) - glfwGetKey(globs.window, GLFW_KEY_A),
        (float)glfwGetKey(globs.window, GLFW_KEY_W) - glfwGetKey(globs.window, GLFW_KEY_S)
    };
    float scale = glfwGetKey(globs.window, GLFW_KEY_E) - glfwGetKey(globs.window, GLFW_KEY_Q);
    info->camHeight += scale * info->camHeight * globs.dt;
    info->camPos += info->camHeight * globs.dt * move * 0.5;




    Mat4f proj;
    float aspect = width/height;
    float halfHeight = info->camHeight / 2;
    matrixOrtho(-aspect * halfHeight, aspect * halfHeight, -halfHeight, halfHeight, 0, 100, proj);
    Mat4f view;
    matrixTranslation(info->camPos.x, info->camPos.y, 0, view);
    matrixInverse(view, view);
    Mat4f vp = view * proj;

    // point dragging and finding which is hovered
    for(int i = 0; i < info->pathPtCount; i++) {
        a = blu_areaMake(str_format(globs.scratch, STR("%i"), i),
            blu_areaFlags_DRAW_BACKGROUND |
            blu_areaFlags_FLOATING |
            blu_areaFlags_CLICKABLE |
            blu_areaFlags_HOVER_ANIM);
        blu_WidgetInteraction inter = blu_interactionFromWidget(a);
        float size = lerp(10, 14, a->target_hoverAnim);
        a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, size };
        a->style.sizes[blu_axis_Y] = { blu_sizeKind_PX, size };

        V2f drag = inter.dragDelta;
        drag /= V2f{ width, -height };
        drag *= V2f{ info->camHeight/2, info->camHeight * aspect/2 };
        info->path[i] += drag;
        // TODO: dragging is completely incorrect

        V2f pt = info->path[i];
        V4f temp = V4f(pt.x, pt.y, 0, 1) * vp; // move to screenspace (-1 to 1 inside of FB area)
        pt = V2f{ temp.x, temp.y } / 2 + V2f(0.5); // shift 0 to 1 range
        pt *= V2f{ width, height }; // move to fb space
        a->offset = { pt.x, height-pt.y }; // invert Y bc ui uses down+
        a->offset -= (a->rect.end - a->rect.start) / 2;

        a->style.backgroundColor = col_green;

        if(inter.held) {
            info->pathDirty = true;
        }
    }


    // DRAW PATHS ==========================================================================================
    gfx_Pass* p = gfx_registerClearPass(col_darkBlue, fb);
    p = gfx_registerPass();
    p->target = fb;
    p->shader = globs.lineShader;
    p->passUniforms.resolution = { width, height };
    p->drawIndexed = false;
    p->passUniforms.vp = vp;

    U32 renderPointCount = info->pathPtCount/4 * PATH_BEZIERSUBDIVCOUNT;
    // Only recalculate curve data if this flag gets set
    // TODO: this will miss one segment between two splines, which looks really nasty. plz fix
    if(info->pathDirty) {
        if(renderPointCount > 0) {
            LineVert* pts = BUMP_PUSH_ARR(globs.scratch, renderPointCount + 2, LineVert);
            int ptCount = 0;
            V2f pt = info->path[0] + info->path[0] - info->path[1];
            ARR_APPEND(pts, ptCount, (LineVert{ V4f(pt.x, pt.y, 0, 1), col_white })); // 1st miter to point to 1st control
            for(float i = 0; i <= renderPointCount; i += 1) {
                V2f sample = bezierSample(info->path, info->pathPtCount, i/(float)renderPointCount);
                ARR_APPEND(pts, ptCount, (LineVert{ V4f(sample.x, sample.y, 0, 1), col_white }));
            }
            pt = info->path[info->pathPtCount-1] + info->path[info->pathPtCount-1] - info->path[info->pathPtCount-2];
            ARR_APPEND(pts, ptCount, (LineVert{ V4f(pt.x, pt.y, 0, 1), col_white })); // end miter points to last control point
            gfx_updateSSBO(info->pathSSBO, pts, (ptCount) * sizeof(LineVert), false);
        }
        info->pathDirty = false;
    }
    gfx_UniformBlock* b = gfx_registerCall(p);
    b->thickness = 3;
    b->ssbo = info->pathSSBO;
    b->vertCount = 6*(renderPointCount+3-1);
}


// expects table to be empty
void loadReplay(net_Table* table) {
    U64 size;
    U8* f = loadFileToBuffer("test.log", true, &size, globs.scratch);
    ASSERT(f);

    U8* c = f;
    U8* lineStart = c;
    while((c - f) < size) {
        if(*c != '\n') { c++; continue; }

        str line = { lineStart, c-lineStart };
        U32 spCount;
        str* split;
        str_split(line, ' ', globs.scratch, &spCount, &split);
        ASSERT(spCount > 0);

        // [prop/update] [name]
        // prop:                [type]
        // update:              [timestamp] [value]

        if(split[0].length == 1 && split[0].chars[0] == 'p') {
            net_Prop* p = ARR_APPEND(table->props, table->propCount, net_Prop());
            p->name = str_copy(split[1], globs.ctrlInfo.replayArena);

            str typeStr = split[2];
            if(str_compare(typeStr, STR("s32"))) { p->type = net_propType_S32; }
            else if(str_compare(typeStr, STR("f64"))) { p->type = net_propType_F64; }
            else if(str_compare(typeStr, STR("bool"))) { p->type = net_propType_BOOL; }
            else if(str_compare(typeStr, STR("str"))) { p->type = net_propType_STR; }
            else { ASSERT(false); }
        }
        else if(split[0].length == 1 && split[0].chars[0] == 'u') {
            net_PropSample* sample = BUMP_PUSH_NEW(globs.ctrlInfo.replayArena, net_PropSample);

            net_Prop* p = net_getProp(split[1], table);
            ASSERT(p);
            sample->next = p->firstPt;
            p->firstPt = sample;

            sample->timeStamp = atof((const char*)split[2].chars);

            const char* arg = (const char*)split[3].chars;
            if(p->type == net_propType_S32) { sample->s32 = atoi(arg); }
            else if(p->type == net_propType_F64) { sample->f64 = atof(arg); }
            else if(p->type == net_propType_STR) { sample->str = str_copy(split[3], globs.ctrlInfo.replayArena); }
            else if(p->type == net_propType_BOOL) {
                bool end;
                if(str_compare(split[3], STR("true"))) { sample->boo = true; }
                else if(str_compare(split[3], STR("false"))) { sample->boo = false; }
                else { ASSERT(false); }
            }
            else { ASSERT(false); }
        }

        c++;
        lineStart = c;
    }
}

// TODO: add pause/play button

void draw_controls(ControlsInfo* info) {

    blu_Area* a;

    a = blu_areaMake("controlInfo", 0);
    blu_style_childLayoutAxis(blu_axis_X, &a->style);
    blu_style_sizeY({ blu_sizeKind_REMAINDER, 0 }, &a->style);
    blu_parentScope(a) {
        blu_styleScope(blu_Style()) {
        blu_style_childLayoutAxis(blu_axis_Y);


            a = blu_areaMake("left", blu_areaFlags_DRAW_BACKGROUND);
            blu_style_sizeX({ blu_sizeKind_PX, 75}, &a->style);
            blu_parentScope(a) {
                blu_styleScope(blu_Style()) {
                    blu_style_sizeX({blu_sizeKind_PERCENT, 1});
                    blu_style_sizeY({blu_sizeKind_REMAINDER, 1});

                    bool resetTable = false;
                    if(makeButton(STR("LIVE"), !info->usingSockets?col_darkGray:col_darkBlue, col_lightGray).clicked) {
                        info->usingSockets = true;
                        resetTable = true;
                    }

                    if(makeButton(STR("REPLAY"), info->usingSockets?col_darkGray:col_darkBlue, col_lightGray).clicked) {
                        info->usingSockets = false;
                        resetTable = true;
                    }

                    if(resetTable) {
                        memset(&globs.ctrlInfo.replayTable.props, 0, sizeof(globs.ctrlInfo.replayTable.props));
                        globs.ctrlInfo.replayTable.propCount = 0;
                        bump_clear(globs.ctrlInfo.replayArena);

                        globs.curTime = 0;
                        globs.table = globs.ctrlInfo.replayTable;
                    }
                    // CLEANUP: only trigger on state change, and move to after UI build ends
                }
            }

            a = blu_areaMake("right", blu_areaFlags_DRAW_BACKGROUND);
            blu_style_sizeX({ blu_sizeKind_REMAINDER, 0}, &a->style);

            blu_parentScope(a) {
                blu_styleScope(blu_Style()) {
                blu_style_sizeY({ blu_sizeKind_TEXT, 0 });
                blu_style_sizeX({ blu_sizeKind_REMAINDER, 0 });
                blu_style_childLayoutAxis(blu_axis_X);


                    if(info->usingSockets) {
                        a = blu_areaMake(STR("connectionStatus"), blu_areaFlags_DRAW_BACKGROUND);
                        blu_style_sizeX({ blu_sizeKind_REMAINDER, 0}, &a->style);
                        blu_parentScope(a) {
                            a = blu_areaMake(STR("label"), blu_areaFlags_DRAW_TEXT);
                            blu_areaAddDisplayStr(a, STR("Network status: "));
                            blu_style_sizeX({ blu_sizeKind_TEXT, 0 }, &a->style);

                            a = blu_areaMake(STR("box"), blu_areaFlags_DRAW_BACKGROUND);
                            a->style.backgroundColor = col_red;
                            a->style.sizes[blu_axis_X] = { blu_sizeKind_REMAINDER, 0 };
                            a->style.cornerRadius = 2;
                            if(net_getConnected(&globs.table)) {
                                a->style.backgroundColor = col_green; }
                        } // end connection parent


                        a = blu_areaMake("simSwitchParent", 0);
                        a->style.sizes[blu_axis_X] = { blu_sizeKind_PERCENT, .2 };
                        blu_parentScope(a) {
                            blu_styleScope(blu_Style()) {
                            blu_style_sizeX({ blu_sizeKind_REMAINDER, 0 });
                            blu_style_style(&borderStyle);

                                bool clicked = false;
                                if(makeButton(STR("sim"), info->sim? col_darkGray:col_darkBlue, col_lightGray).clicked) {
                                    info->sim = true;
                                }
                                if(makeButton(STR("real"), !info->sim? col_darkGray:col_darkBlue, col_lightGray).clicked) {
                                    info->sim = false;
                                }
                            }
                        }

                    }
                    else {
                        if(makeButton(STR("load"), col_lightGray).clicked) {
                            bump_clear(globs.ctrlInfo.replayArena);
                            memset(&globs.ctrlInfo.replayTable.props, 0, sizeof(globs.ctrlInfo.replayTable.props));
                            globs.ctrlInfo.replayTable.propCount = 0;

                            loadReplay(&globs.ctrlInfo.replayTable);

                            globs.curTime = 0;
                            globs.table = globs.ctrlInfo.replayTable;
                            globs.ctrlInfo.refreshTableFlag = true;
                        }

                        a = blu_areaMake("nav", blu_areaFlags_DRAW_BACKGROUND |
                                                blu_areaFlags_HOVER_ANIM |
                                                blu_areaFlags_CLICKABLE |
                                                blu_areaFlags_DRAW_TEXT |
                                                blu_areaFlags_CENTER_TEXT);
                        a->style.backgroundColor = v4f_lerp(col_darkBlue, col_darkGray, a->target_hoverAnim);
                        blu_WidgetInteraction i = blu_interactionFromWidget(a);
                        if(i.held) {
                            a->cursor = blu_cursor_resizeH;
                            globs.curTime += i.dragDelta.x * 0.01f;
                            globs.ctrlInfo.refreshTableFlag = true;
                        }
                        blu_areaAddDisplayStr(a, str_format(globs.scratch, STR("%f"), globs.curTime));
                    }
                }
            } // end right side
        }
    } // end control info
};




void draw_powerIndicators(PowerIndicatorInfo* info) {

    blu_Area* a = makeScrollArea(&info->scrollPosition);
    blu_parentScope(a) {
        blu_styleScope(blu_Style()) {
        blu_style_sizeX({ blu_sizeKind_PERCENT, 1 });
        blu_style_sizeY({ blu_sizeKind_PERCENT, 1 });

            for (int i = 0; i < POWER_INDICATOR_COUNT; i++) {
                net_PropSample* s = nullptr;
                if(info->keys[i].str.length > 0) {
                    net_Prop* p = net_getProp(info->keys[i].str, net_propType_F64, &globs.table);
                    if(p) { s = p->firstPt; }
                }
                bool fadeText = (!s || !net_getConnected(&globs.table));

                str indexStr = str_format(globs.scratch, STR("%i"), i);
                a = blu_areaMake(indexStr, blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_HOVER_ANIM | blu_areaFlags_CLICKABLE | blu_areaFlags_DROP_EVENTS);
                blu_style_style(&borderStyle, &a->style);
                blu_style_childLayoutAxis(blu_axis_X, &a->style);
                blu_style_backgroundColor(col_darkBlue, &a->style);
                blu_style_sizeY({ blu_sizeKind_PX, 40 }, &a->style);


                a->dropTypeMask = dropMasks_NT_PROP;
                blu_WidgetInteraction inter = blu_interactionFromWidget(a);
                if(inter.dropped) {
                    NTKey* k = &info->keys[i];
                    *k = NTKey();
                    net_Prop* p = (net_Prop*)inter.dropVal;
                    ASSERT(p->name.length < 255);
                    k->str = str_copy(p->name, k->chars);
                }


                if(inter.hovered && inter.dropType) {
                    float t = a->target_hoverAnim;
                    blu_style_backgroundColor(v4f_lerp(a->style.backgroundColor, col_lightGray, t), &a->style);
                }
                else if(inter.hovered && !inter.dropType && info->keys[i].str.length > 0) {
                    fadeText = true;
                    blu_parentScope(a) {
                        a = blu_areaMake("X", blu_areaFlags_DRAW_TEXT | blu_areaFlags_CENTER_TEXT);
                        blu_areaAddDisplayStr(a, "X");
                        blu_style_sizeX({ blu_sizeKind_PERCENT, 1 });
                        if(inter.clicked) {
                            info->keys[i] = NTKey();
                        }
                    }
                }

                blu_parentScope(a) {

                    float w = a->calculatedSizes[blu_axis_X];
                    a = blu_areaMake("indicator", blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_FLOATING);
                    a->offset = { 0, 0 };
                    a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 0 };

                    blu_style_backgroundColor(col_red, &a->style);
                    if(fadeText) { a->style.backgroundColor *= col_disconnect; }

                    if(s) {
                        float val = (F32)s->f64;
                        float leftShift = 0;
                        float barSize = val * (w/2);

                        if(val < 0) {
                            leftShift = barSize;
                            barSize *= -1;
                        }

                        a->offset = { w/2 + leftShift, 0 };
                        blu_style_sizeX({ blu_sizeKind_PX, barSize }, &a->style);
                    }


                    a = blu_areaMake("key", blu_areaFlags_DRAW_TEXT | blu_areaFlags_FLOATING);
                    a->offset = V2f(0, 0);
                    blu_areaAddDisplayStr(a, info->keys[i].str);
                    if(fadeText) { a->style.textColor *= col_disconnect; }


                    if(s) {
                        int val = (int)(s->f64 * 100);
                        a = blu_areaMake("value", blu_areaFlags_DRAW_TEXT | blu_areaFlags_CENTER_TEXT | blu_areaFlags_FLOATING);
                        blu_areaAddDisplayStr(a, str_format(globs.scratch, STR("%i%%"), val));
                        if(fadeText) { a->style.textColor *= col_disconnect; }
                    }


                } // end per elem parent
            } // end loop
        }
    } // end scroll view
}





void draw_swerveDrive(SwerveDriveInfo* info, gfx_Framebuffer* target) {

    blu_Area* a = blu_areaMake("swerveDisplay", blu_areaFlags_DRAW_TEXTURE);
    a->texture = target->texture;

    areaAddFB(a, target);

    gfx_Pass* p = gfx_registerClearPass(col_darkBlue, target);

    p = gfx_registerPass();
    p->target = target;
    p->shader = globs.sceneShader2d;

    float aspect = ((float)target->texture->width/target->texture->height);
    float size = 4;
    float width = size * aspect;
    float height = size;

    if(aspect < 1) {
        width = 4;
        height = width * (1/aspect);
    }
    matrixOrtho(-width/2, width/2, -height/2, height/2, 0, 10000, p->passUniforms.vp);


    V2f translations[] = {
        { -1, 1 },
        { 1, 1},
        { -1, -1},
        { 1, -1 }
    };

    net_PropSample* rotationProps[] = {
        net_getSample(STR("FLSteerPos"), net_propType_F64, &globs.table),
        net_getSample(STR("FRSteerPos"), net_propType_F64, &globs.table),
        net_getSample(STR("BLSteerPos"), net_propType_F64, &globs.table),
        net_getSample(STR("BRSteerPos"), net_propType_F64, &globs.table)
    };
    net_PropSample* distProps[] = {
        net_getSample(STR("FLDrivePos"), net_propType_F64, &globs.table),
        net_getSample(STR("FRDrivePos"), net_propType_F64, &globs.table),
        net_getSample(STR("BLDrivePos"), net_propType_F64, &globs.table),
        net_getSample(STR("BRDrivePos"), net_propType_F64, &globs.table)
    };

    net_PropSample* angleSample = net_getSample(STR("yaw"), net_propType_F64, &globs.table);
    float angle = 90;
    if(angleSample) { angle += (F32)(angleSample->f64); }

    Mat4f temp;

    for(int i = 0; i < 4; i++) {

        Mat4f t;
        matrixTranslation(translations[i].x, translations[i].y, 0, t);

        if(rotationProps[i]) {
            matrixZRotation(-(F32)rotationProps[i]->f64 * 360, temp);
            t = temp * t;
        }

        matrixZRotation(-angle, temp);
        t = t * temp;

        // wheel
        gfx_UniformBlock* b = gfx_registerCall(p);
        b->texture = globs.wheelTex;
        b->model = t;

        // tread
        matrixScale(0.25, 0.95, 0, temp);

        float pos = 0;
        if(distProps[i]) {
            pos = (F32)distProps[i]->f64 * 0.1f;
        }

        b = gfx_registerCall(p);
        b->texture = globs.treadTex;
        b->model = temp * t;
        b->srcStart = V2f(0, pos);
        b->srcEnd = V2f(1, pos - 1);


        // forward arrow
        b = gfx_registerCall(p);
        b->texture = globs.solidTex;
        matrixTranslation(0, 0.75, 0, temp);
        t = temp * t;
        matrixScale(0.1, 0.1, 0, temp);
        t = temp * t;
        b->model = t;

    }

    gfx_UniformBlock* b = gfx_registerCall(p);
    b->texture = globs.arrowTex;
    matrixZRotation(-angle, b->model);

    matrixScale((F32)globs.arrowTex->width / globs.arrowTex->height, 1, 1, temp);
    b->model = temp * b->model;
}






void draw_graph2d(Graph2dInfo* info, gfx_Framebuffer* target) {

    blu_Area* a = blu_areaMake("graph2d", blu_areaFlags_DRAW_BACKGROUND);
    blu_style_childLayoutAxis(blu_axis_Y, &a->style);

    blu_parentScope(a) {

        a = blu_areaMake("upperBit", blu_areaFlags_DRAW_TEXTURE | blu_areaFlags_CLICKABLE | blu_areaFlags_SCROLLABLE);
        blu_style_sizeY({blu_sizeKind_REMAINDER, 0}, &a->style);
        areaAddFB(a, target);
        float width = target->texture->width;
        float height = target->texture->height;

        gfx_registerClearPass(col_darkBlue, target);

        gfx_Pass* p = gfx_registerPass();
        p->target = target;
        p->shader = globs.lineShader;
        p->passUniforms.resolution = a->rect.end - a->rect.start;
        p->drawIndexed = false;

        blu_WidgetInteraction inter = blu_interactionFromWidget(a);

        if(height != 0 && inter.hovered) {
            float viewSize = info->top - info->bottom;
            V2f change = V2f(inter.dragDelta.y * (viewSize / height));

            float scaleChange = inter.scrollDelta * viewSize * 0.05;
            float pct = (inter.mousePos.y / height);

            change += V2f(pct * scaleChange, (1-pct) * -scaleChange);

            info->top += change.x;
            info->bottom += change.y;
        }
        matrixOrtho(0, 1, info->bottom, info->top, 0, 100, p->passUniforms.vp);

        float pointGap = 1 / (float)GRAPH2D_VCOUNT;
        float sampleGap = GRAPH2D_SAMPLE_WINDOW / GRAPH2D_VCOUNT;



        // time lines
        {
            int lineCount = 10;
            int vertCount = lineCount * 2;
            LineVert* gridPts = BUMP_PUSH_ARR(globs.scratch, vertCount+4, LineVert);
            float ypts[] = { info->bottom, info->top };

            for(int i = 0; i < lineCount; i++) {
                LineVert* v = &gridPts[i*2+1];
                bool even = i%2 == 0;

                float x = ((i-fmodf(globs.curTime, 1)) / GRAPH2D_SAMPLE_WINDOW);
                V4f color = v4f_lerp(col_darkGray, col_darkBlue, (1-x)/2); // gradient
                v[0] = { V4f(x, ypts[even], 0, 1), color };
                v[1] = { V4f(x, ypts[!even], 0, 1), color };
            }

            // hover line
            float x = 100;
            if(inter.hovered) { x = inter.mousePos.x / width; }
            gridPts[vertCount+1] = { V4f(x, ypts[1], 0, 1), col_darkGray };
            gridPts[vertCount+2] = { V4f(x, ypts[0], 0, 1), col_darkGray };
            gridPts[vertCount+3] = { V4f(0, 0, 0, 1), col_darkGray };

            gfx_updateSSBO(info->timeVerts, gridPts, sizeof(LineVert) * (vertCount+4), true);
            gfx_UniformBlock* b = gfx_registerCall(p);
            b->thickness = 1;
            b->ssbo = info->timeVerts;
            b->vertCount = 6*(vertCount+2-1);
        }

        // value lines
        {
            int lineCount = 3;
            int vertCount = lineCount * 2;
            LineVert* gridPts = BUMP_PUSH_ARR(globs.scratch, vertCount+2, LineVert);
            float xpts[] = { -0.1, 1.1 };

            for(int i = 0; i < lineCount; i++) {
                LineVert* v = &gridPts[i*2+1];
                bool even = i%2 == 0;
                float y = i - (lineCount/2);
                v[0].pos = V4f(xpts[even], y, 0, 1);
                v[0].color = col_darkGray;
                v[1].pos = V4f(xpts[!even], y, 0, 1);
                v[1].color = col_darkGray;
            }
            gridPts[0] = { V4f(), col_darkGray };
            gridPts[vertCount+1] = { V4f(), col_darkGray };
            gfx_updateSSBO(info->gridVerts, gridPts, sizeof(LineVert) * (vertCount+2), true);

            gfx_UniformBlock* b = gfx_registerCall(p);
            b->color = col_darkGray;
            b->thickness = 1;
            b->ssbo = info->gridVerts;
            b->vertCount = 6*(vertCount-1);
        }

        // TODO: value labels

        blu_styleScope(blu_Style()) {
        blu_style_sizeX({ blu_sizeKind_TEXT, 0 });
        blu_style_sizeY({ blu_sizeKind_TEXT, 0 });
            float timeOffset = fmodf(globs.curTime, 1);

            for(int i = 0; i < 10; i++) {
                a = blu_areaMake(str_format(globs.scratch, STR("%i"), i), blu_areaFlags_FLOATING | blu_areaFlags_DRAW_TEXT | blu_areaFlags_CENTER_TEXT);
                a->offset =  { (1-((i+timeOffset) / GRAPH2D_SAMPLE_WINDOW)) * width - a->calculatedSizes[blu_axis_X] / 2, height - BLU_FONT_SIZE };
                a->textScale = 0.5f;
                blu_areaAddDisplayStr(a, str_format(globs.scratch, STR("%i"), (int)(globs.curTime-i)));
            }

            if(inter.hovered) {
                a = blu_areaMake("hoverLabel", blu_areaFlags_FLOATING | blu_areaFlags_DRAW_TEXT | blu_areaFlags_CENTER_TEXT);
                a->offset =  { inter.mousePos.x - a->calculatedSizes[blu_axis_X] / 2, height - BLU_FONT_SIZE };
                a->textScale = 0.6f;
                blu_areaAddDisplayStr(a, str_format(globs.scratch, STR("%f"), globs.curTime - GRAPH2D_SAMPLE_WINDOW * (1-(inter.mousePos.x / width))));
            }
        }




        LineVert* pts = BUMP_PUSH_ARR(globs.scratch, GRAPH2D_VCOUNT, LineVert);
        for(int i = 0; i < GRAPH2D_LINECOUNT; i++) {
            if(info->keys[i].str.length == 0) { continue; }

            // TODO: int/bool graphing

            for(int j = 0; j < GRAPH2D_VCOUNT; j++) {
                V4f color = info->colors[i];

                float sHeight = 0;
                // TODO: inline traversal instead of restarting constantly
                float sampleTime = globs.curTime - sampleGap*j;
                net_PropSample* sample = net_getSample(info->keys[i].str, net_propType_F64, sampleTime, &globs.table);
                if(sample) { sHeight = (float)sample->f64; }
                else { color *= col_disconnect; }

                sample = net_getSample(STR("/connected"), net_propType_BOOL, sampleTime, &globs.table);
                bool connected = false;
                if(sample && sample->boo) { connected = true; }
                if(sample && !connected) { color *= col_disconnect; }

                pts[j] = { V4f(1-(j*pointGap), sHeight, 0, 1), color };
            }

            gfx_updateSSBO(info->lineVerts[i], pts, sizeof(LineVert) * GRAPH2D_VCOUNT, true);

            gfx_UniformBlock* b = gfx_registerCall(p);
            b->color = info->colors[i];
            b->thickness = 2;
            b->ssbo = info->lineVerts[i];
            b->vertCount = 6*(GRAPH2D_VCOUNT-1);
        }

        a = blu_areaMake("lowerBit", blu_areaFlags_FLOATING);
        blu_style_sizeY({ blu_sizeKind_PERCENT, 1 }, &a->style);
        blu_style_childLayoutAxis(blu_axis_Y, &a->style);
        blu_parentScope(a) {
            blu_styleScope(blu_Style()) {
            blu_style_sizeY({ blu_sizeKind_TEXT, 0 });
            blu_style_sizeX({ blu_sizeKind_CHILDSUM, 0 });
            blu_style_backgroundColor(col_darkGray);
            blu_style_cornerRadius(5);
            blu_style_childLayoutAxis(blu_axis_X);

                for(int i = 0; i < GRAPH2D_LINECOUNT; i++) {
                    if(!info->keys[i].chars) { continue; }

                    a = blu_areaMake(str_format(globs.scratch, STR("thing %i"), i),
                        blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DROP_EVENTS | blu_areaFlags_CLICKABLE | blu_areaFlags_HOVER_ANIM);
                    inter = blu_interactionFromWidget(a);

                    bool fadeChildren = false;
                    if(!net_getConnected(&globs.table)) { fadeChildren = true; }
                    if(inter.hovered && !inter.dropType) { fadeChildren = true; }

                    float hoverTarget = a->target_hoverAnim;
                    a->style.backgroundColor = v4f_lerp(a->style.backgroundColor, col_lightGray, hoverTarget);
                    if(fadeChildren) { a->style.backgroundColor *= col_disconnect; }

                    a->dropTypeMask = dropMasks_NT_PROP;
                    if(inter.dropped) {
                        net_Prop* prop = ((net_Prop*)(inter.dropVal));
                        if(prop->type == net_propType_F64) {
                            ASSERT(prop->name.length < 255);
                            info->keys[i].str = str_copy(prop->name, info->keys[i].chars);
                        }
                    }

                    if(inter.clicked && !inter.dropType) {
                        memset(&info->keys[i], 0, sizeof(NTKey));
                    }

                    blu_parentScope(a) {

                        a = blu_areaMake("color", blu_areaFlags_DRAW_BACKGROUND);
                        a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 14 };
                        a->style.backgroundColor = info->colors[i];
                        if(fadeChildren) { a->style.backgroundColor *= col_disconnect; }

                        a = blu_areaMake("text", blu_areaFlags_DRAW_TEXT);
                        blu_areaAddDisplayStr(a, info->keys[i].str);
                        a->style.sizes[blu_axis_X] = { blu_sizeKind_TEXT, 0 };
                        if(fadeChildren) { a->style.textColor *= col_disconnect; }


                        if(inter.hovered && info->keys[i].str.length == 0) {
                            a = blu_areaMake("spacer", 0);
                            a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, hoverTarget * 10 };
                        }
                        if(inter.hovered && !inter.dropType) {

                            a = blu_areaMake("X", blu_areaFlags_DRAW_TEXT);
                            blu_areaAddDisplayStr(a, "X");
                            a->style.sizes[blu_axis_X] = { blu_sizeKind_TEXT, 0 };
                        }

                    }

                }
            }
        }
    } // end area
};



// TODO: make controls not apply unless field is "selected"
// TODO: robot follow mode

void draw_field(FieldInfo* info, gfx_Framebuffer* fb) {

    V4f mVec = { 0, 0, 0, 0 };

    F32 moveSpeed = 3;
    GLFWwindow* window = globs.window;
    float dt = globs.dt;
    mVec.x += (glfwGetKey(window, GLFW_KEY_D) - glfwGetKey(window, GLFW_KEY_A)) * moveSpeed * dt;
    mVec.z -= (glfwGetKey(window, GLFW_KEY_W) - glfwGetKey(window, GLFW_KEY_S)) * moveSpeed * dt;
    mVec.y += (glfwGetKey(window, GLFW_KEY_E) - glfwGetKey(window, GLFW_KEY_Q)) * moveSpeed * dt;

    Mat4f ry;
    Mat4f rx;
    matrixYRotation(info->camTransform.ry, ry);
    matrixXRotation(info->camTransform.rx, rx);
    mVec = mVec * (rx * ry);

    info->camTarget.x += mVec.x;
    info->camTarget.y += mVec.y;
    info->camTarget.z += mVec.z;



    net_PropSample* posX = net_getSample(STR("posX"), net_propType_F64, &globs.table);
    net_PropSample* posY = net_getSample(STR("posY"), net_propType_F64, &globs.table);
    net_PropSample* yaw = net_getSample(STR("yaw"), net_propType_F64, &globs.table);

    net_PropSample* estX = net_getSample(STR("estX"), net_propType_F64, &globs.table);
    net_PropSample* estY = net_getSample(STR("estY"), net_propType_F64, &globs.table);

    net_PropSample* targetX = net_getSample(STR("targetX"), net_propType_F64, &globs.table);
    net_PropSample* targetY = net_getSample(STR("targetY"), net_propType_F64, &globs.table);
    net_PropSample* targetA = net_getSample(STR("targetAngle"), net_propType_F64, &globs.table);

    Transform robotTransform = Transform();
    if(posX && posY && yaw) {
        robotTransform.x = (F32)posX->f64;
        robotTransform.z = -(F32)posY->f64;
        robotTransform.ry = -(F32)yaw->f64;
    }

    Transform estimateTransform = Transform();
    if(estX && estY && yaw) {
        estimateTransform.x = (F32)estX->f64;
        estimateTransform.z = -(F32)estY->f64;
        estimateTransform.ry = -(F32)yaw->f64;
    }

    Transform targetTransform = Transform();
    if(targetX && targetY && targetA) {
        targetTransform.x = (F32)targetX->f64;
        targetTransform.z = -(F32)targetY->f64;
        targetTransform.ry = -(F32)targetA->f64;
    }




    blu_Area* a = blu_areaMake("fieldDisplay", blu_areaFlags_DRAW_TEXTURE | blu_areaFlags_CLICKABLE);
    a->texture = fb->texture;

    V2f delta = blu_interactionFromWidget(a).dragDelta;
    info->camTarget.ry -= delta.x * 0.5f;
    info->camTarget.rx -= delta.y * 0.5f;

    areaAddFB(a, fb);

    Mat4f proj = Mat4f(1.0f);
    matrixPerspective(90, (float)fb->texture->width / fb->texture->height, 0.01, 1000000, proj);


    blu_parentScope(a) {
        blu_styleScope(blu_Style()) {
        blu_style_sizeX({ blu_sizeKind_TEXT, 0 });
        blu_style_sizeY({ blu_sizeKind_TEXT, 0 });
        blu_style_backgroundColor(col_darkGray * V4f(1, 1, 1, 0.75f));
        blu_style_style(&borderStyle);

            a = blu_areaMake("buttonParent", blu_areaFlags_FLOATING);
            blu_style_sizeX({ blu_sizeKind_PERCENT, 1 }, &a->style);
            blu_style_childLayoutAxis(blu_axis_X, &a->style);
            a->offset = V2f(5, 5);
            blu_parentScope(a) {

                if(makeButton(STR("Home"), col_white).clicked) {
                    info->camTarget = {
                        0, 6, 0,
                        -90, 0, 0,
                        1, 1, 1
                        };
                }
                if(makeButton(STR("Blue"), col_white).clicked) {
                    info->camTarget = {
                        -4, 7, 0,
                        -75, -90, 0,
                        1, 1, 1
                        };
                }
                if(makeButton(STR("Red"), col_white).clicked) {
                    info->camTarget = {
                        4, 7, 0,
                        -75, 90, 0,
                        1, 1, 1
                        };
                }
            }

            a = blu_areaMake(STR("FPS"), blu_areaFlags_DRAW_TEXT | blu_areaFlags_FLOATING);
            a->offset = V2f(5, fb->texture->height - BLU_FONT_SIZE - 5);
            str n = str_format(globs.scratch, STR("FPS: %f"), 1/globs.dt);
            blu_areaAddDisplayStr(a, n);
        }
    }



    gfx_Pass* p = gfx_registerClearPass(col_darkBlue, fb);


    // CLEANUP: transform lerp func, also quaternions would be cool
    float followStrength = 0.1f;
    info->camTransform.x = lerp(info->camTransform.x, info->camTarget.x, followStrength);
    info->camTransform.y = lerp(info->camTransform.y, info->camTarget.y, followStrength);
    info->camTransform.z = lerp(info->camTransform.z, info->camTarget.z, followStrength);
    info->camTransform.rx = lerp(info->camTransform.rx, info->camTarget.rx, followStrength);
    info->camTransform.ry = lerp(info->camTransform.ry, info->camTarget.ry, followStrength);



    Mat4f view = matrixTransform(info->camTransform);
    matrixInverse(view, view);

    p = gfx_registerPass();
    p->target = fb;
    p->shader = globs.sceneShader3d;
    p->passUniforms.vp = view * proj;


    gfx_UniformBlock* b;


    b = gfx_registerCall(p);
    b->color = V4f(1, 1, 1, 1);
    b->ib = globs.fieldIB;
    b->va = globs.fieldVA;
    b->texture = globs.fieldTex;

    Transform field = Transform(); // TODO: get a field model
    field.sx = 16.4846 / 2;
    field.sz = 8.1026 / 2;
    b->model = matrixTransform(field);

    b = gfx_registerCall(p);
    b->color = col_purple * V4f(1, 1, 1, 0.5);
    b->ib = globs.robotIB;
    b->va = globs.robotVA;
    b->model = matrixTransform(targetTransform);
    b->texture = globs.solidTex;

    b = gfx_registerCall(p);
    b->color = V4f(1, 0, 0, 0.5);
    b->ib = globs.robotIB;
    b->va = globs.robotVA;
    b->model = matrixTransform(estimateTransform);
    b->texture = globs.solidTex;


    b = gfx_registerCall(p);
    b->color = V4f(1, 0, 0, 1);
    b->ib = globs.robotIB;
    b->va = globs.robotVA;
    b->model = matrixTransform(robotTransform);
    b->texture = globs.solidTex;
}









void draw_network(NetInfo* info) {
    blu_Area* a;



    blu_styleScope(blu_Style()) {
        blu_style_sizeX({ blu_sizeKind_PERCENT, 1 });
        blu_style_sizeY({ blu_sizeKind_REMAINDER, 0 });
        a = makeScrollArea(&info->clipPos);
    }

    blu_parentScope(a) {
        blu_styleScope(blu_Style()) {
        blu_style_sizeY({ blu_sizeKind_TEXT, 0 });
        blu_style_childLayoutAxis(blu_axis_X);
        blu_style_backgroundColor(col_darkGray);

            for(int i = 0; i < globs.table.propCount; i++) {
                net_Prop* prop = &globs.table.props[i];
                if(!prop->firstPt) { continue; }

                str quotedName = str_format(globs.scratch, STR("\"%s\""), prop->name);

                blu_Area* parent = blu_areaMake(str_format(globs.scratch, STR("%i"), i), blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_HOVER_ANIM | blu_areaFlags_CLICKABLE);
                blu_style_style(&borderStyle, &parent->style);
                F32 t = parent->target_hoverAnim;
                parent->dropType = dropMasks_NT_PROP;
                parent->dropVal = prop;

                if(blu_interactionFromWidget(parent).held) {
                    t = 1;
                    blu_parentScope(blu_getCursorParent()) {
                        blu_styleScope(blu_Style()) {
                        blu_style_style(&borderStyle);
                        blu_style_borderSize(2);
                        blu_style_sizeX({ blu_sizeKind_TEXT, 0 });
                        blu_style_sizeY({ blu_sizeKind_TEXT, 0 });
                        blu_style_cornerRadius(4);

                            a = blu_areaMake("drag indicator", blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DRAW_TEXT);
                            blu_areaAddDisplayStr(a, quotedName);
                        }
                    }
                }
                parent->style.backgroundColor = v4f_lerp(col_darkBlue, col_darkGray, t);

                blu_parentScope(parent) {
                    blu_styleScope(blu_Style()) {
                    blu_style_sizeX({ blu_sizeKind_PERCENT, 0.5 });
                    blu_style_style(&borderStyle);

                        a = blu_areaMake("label", blu_areaFlags_DRAW_TEXT);
                        blu_areaAddDisplayStr(a, quotedName);

                        bool connected = net_getConnected(&globs.table);
                        if(!connected) {
                            a->style.backgroundColor *= col_disconnect;
                            a->style.textColor *= col_disconnect;
                        }

                        a = blu_areaMake("value", blu_areaFlags_DRAW_TEXT | blu_areaFlags_DRAW_BACKGROUND);
                        a->style.backgroundColor = col_darkGray;

                        BumpAlloc* scratch = globs.scratch;
                        if(prop->type == net_propType_S32) {
                            blu_areaAddDisplayStr(a, str_format(scratch, STR("%i"), (prop->firstPt->s32))); }
                        else if(prop->type == net_propType_F64) {
                            blu_areaAddDisplayStr(a, str_format(scratch, STR("%f"), (prop->firstPt->f64))); }
                        else if(prop->type == net_propType_STR) {
                            blu_areaAddDisplayStr(a, str_format(scratch, STR("\"%s\""), (prop->firstPt->str))); }
                        else if(prop->type == net_propType_BOOL) {
                            blu_areaAddDisplayStr(a, str_format(scratch, STR("%b"), (prop->firstPt->boo)));
                            a->style.backgroundColor = prop->firstPt->boo? col_green : col_red;
                            a->style.cornerRadius = 2;
                            a->style.textColor = col_darkBlue;
                        }

                        if(!connected) {
                            a->style.backgroundColor *= col_disconnect;
                            a->style.textColor *= col_disconnect;
                        }
                    }
                }
            }
        } // end text styling
    } // end of clip
}









// TODO: button texures
void makeViewSrc(const char* name, ViewType type) {

    blu_Area* a = blu_areaMake(name,
        blu_areaFlags_HOVER_ANIM | blu_areaFlags_DRAW_TEXT | blu_areaFlags_CLICKABLE | blu_areaFlags_DRAW_BACKGROUND);
    blu_areaAddDisplayStr(a, name);
    a->textScale = 0.75;
    a->style.backgroundColor = v4f_lerp(col_darkGray, col_lightGray, a->target_hoverAnim);

    a->dropType = dropMasks_VIEW_TYPE;
    a->dropVal = (void*)type;
    if(blu_interactionFromWidget(a).held) {
        blu_parentScope(blu_getCursorParent()) {
            blu_styleScope(blu_Style()) {
            blu_style_style(&borderStyle);
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
            deinitGraph2dInfo(&v->data.graph2dInfo);
        }
        else if(v->type == viewType_paths) {
            deinit_paths(&v->data.pathInfo);
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

void ui_update(BumpAlloc* scratch, GLFWwindow* window, float dt, float curTime) {

    globs.scratch = scratch;
    globs.window = window;
    globs.dt = dt;

    if(globs.ctrlInfo.usingSockets) {
        globs.curTime = curTime;
    }

    blu_Area* a;

    blu_styleScope(blu_Style()) {
    blu_style_backgroundColor(col_darkBlue);
    blu_style_textColor(col_white);
    blu_style_sizeX({ blu_sizeKind_PERCENT, 1 });
    blu_style_sizeY({ blu_sizeKind_PERCENT, 1 });
    blu_style_textPadding(V2f(4, 4));
    blu_style_animationStrength(0.1f);


        a = blu_areaMake(STR("leftBarParent"), blu_areaFlags_DRAW_BACKGROUND);
        a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 50 };
        a->style.childLayoutAxis = blu_axis_Y;

        blu_parentScope(a) {
            blu_styleScope(blu_Style()) {
            blu_style_sizeY({ blu_sizeKind_PX, 50 });
            blu_style_backgroundColor(col_darkGray);
            blu_style_cornerRadius(4);
                makeViewSrc("Field", viewType_field);
                makeViewSrc("Graph", viewType_graph2d);
                makeViewSrc("Swerve", viewType_swerveDrive);
                makeViewSrc("NT", viewType_net);
                makeViewSrc("Power", viewType_powerIndicators);
                makeViewSrc("Ctrls", viewType_controls);
                makeViewSrc("Paths", viewType_paths);
            }
        }

        a = blu_areaMake(STR("leftBarSep"), blu_areaFlags_DRAW_BACKGROUND);
        a->style.backgroundColor = col_black;
        a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 3 };



        a = blu_areaMake("leftParent", 0);
        a->style.sizes[blu_axis_X] = {blu_sizeKind_REMAINDER, 0 };
        a->style.childLayoutAxis = blu_axis_Y;
        blu_parentScope(a) {

            blu_styleScope(blu_Style()) {
            blu_style_sizeY({ blu_sizeKind_REMAINDER, 0 });
                makeView("top", &globs.views[0]);
            }


            a = blu_areaMake("YresizeBar", blu_areaFlags_CLICKABLE | blu_areaFlags_HOVER_ANIM | blu_areaFlags_DRAW_BACKGROUND);
            a->style.sizes[blu_axis_Y] = { blu_sizeKind_PX, 3 };

            blu_WidgetInteraction inter = blu_interactionFromWidget(a);
            float t = inter.held? 1 : a->target_hoverAnim;
            a->style.backgroundColor = v4f_lerp(col_black, col_white, t);
            globs.downSizeL += -inter.dragDelta.y;
            a->cursor = blu_cursor_resizeV;

            blu_styleScope(blu_Style()) {
            blu_style_sizeY({blu_sizeKind_PX, globs.downSizeL });
                makeView("bottom", &globs.views[1]);
            }
        }

        // TODO: everything is lagging by a frame

        a = blu_areaMake("XresizeBar", blu_areaFlags_CLICKABLE | blu_areaFlags_HOVER_ANIM | blu_areaFlags_DRAW_BACKGROUND);
        a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 3 };

        blu_WidgetInteraction inter = blu_interactionFromWidget(a);
        float t = inter.held? 1 : a->target_hoverAnim;
        a->style.backgroundColor = v4f_lerp(col_black, col_white, t);
        globs.rightSize += -inter.dragDelta.x;
        a->cursor = blu_cursor_resizeH;

        a = blu_areaMake("rightParent", 0);
        a->style.sizes[blu_axis_X] = {blu_sizeKind_PX, globs.rightSize };
        a->style.childLayoutAxis = blu_axis_Y;

        blu_parentScope(a) {

            blu_styleScope(blu_Style()) {
            blu_style_sizeY({ blu_sizeKind_REMAINDER, 0 });
                makeView("top", &globs.views[2]);
            }

            a = blu_areaMake("YresizeBar", blu_areaFlags_CLICKABLE | blu_areaFlags_HOVER_ANIM | blu_areaFlags_DRAW_BACKGROUND);
            a->style.sizes[blu_axis_Y] = { blu_sizeKind_PX, 3 };

            blu_WidgetInteraction inter = blu_interactionFromWidget(a);
            float t = inter.held? 1 : a->target_hoverAnim;
            a->style.backgroundColor = v4f_lerp(col_black, col_white, t);
            globs.downSizeR += -inter.dragDelta.y;
            a->cursor = blu_cursor_resizeV;

            blu_styleScope(blu_Style()) {
            blu_style_sizeY({blu_sizeKind_PX, globs.downSizeR });
                makeView("bottom", &globs.views[3]);
            }
        }
    }



    if(!globs.ctrlInfo.usingSockets) {
        if(globs.ctrlInfo.refreshTableFlag) {
            for(int i = 0; i < globs.ctrlInfo.replayTable.propCount; i++) {
                net_PropSample* sample = globs.ctrlInfo.replayTable.props[i].firstPt;

                // CLEANUP: inline traversal
                while(sample) {
                    if(sample->timeStamp < globs.curTime) {
                        break; }
                    sample = sample->next;
                }
                globs.table.props[i].firstPt = sample;
            }
        }
    }
    globs.ctrlInfo.refreshTableFlag = false;



    str s = { nullptr, 0 };
    if(globs.ctrlInfo.usingSockets) {
        s = globs.ctrlInfo.sim? STR("localhost") : STR("10.45.36.2");
    }
    nets_update(&globs.table, (F32)curTime, s);

}