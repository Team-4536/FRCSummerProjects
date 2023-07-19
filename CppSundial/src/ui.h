#pragma once

#include "graphics.h"
#include "base/utils.h"
#include "blue/blue.h"
#include "GLAD/gl.h"
#include "GLFW/glfw3.h"
#include "network/network.h"
#include "colors.h"
#include "stb_image/stb_image.h"

#include <cstring>



#define GRAPH2D_VCOUNT 700
#define GRAPH2D_LINECOUNT 6
struct Graph2dInfo {

    float vals[GRAPH2D_LINECOUNT][GRAPH2D_VCOUNT] = { 0 };
    U8 connectionVals[GRAPH2D_LINECOUNT][GRAPH2D_VCOUNT] = { 0 };
    str keys[GRAPH2D_LINECOUNT] = { 0 };
    V4f colors[GRAPH2D_LINECOUNT];

    float yScale = 1;
    float yOffset = 0;
};
void draw_graph2d(Graph2dInfo* info, gfx_Framebuffer* target, float dt, BumpAlloc* scratch);
void initGraph2dInfo(Graph2dInfo* info, BumpAlloc* resArena) {

    info->colors[0] = col_green;
    info->keys[0] = str_copy(STR("FLSteerSpeed"), resArena); // TODO: access by hash key instead of str
    // TODO: after NT reset, graph ptrs are invalid
    // TODO: memory leak here lol

    info->colors[1] = col_red;
    info->keys[1] = str_copy(STR("FLSteerPos"), resArena);

    info->colors[2] = col_purple;
    info->keys[2] = str_copy(STR("FRSteerSpeed"), resArena);

    info->colors[3] = col_yellow;
    info->keys[3] = str_copy(STR("FRSteerPos"), resArena);
}

struct FieldInfo {
    Transform camTransform;
    Transform camTarget;
};
void draw_field(FieldInfo* info, gfx_Framebuffer* fb, float dt, GLFWwindow* window);

struct NetInfo {
    float clipSize = 0;
    float clipPos = 0;
    float clipMax = 1000;
    // TODO: adaptive max size
};
void draw_network(NetInfo* info, float dt, BumpAlloc* scratch);

struct SwerveDriveInfo {

};
void draw_swerveDrive(SwerveDriveInfo* info, gfx_Framebuffer* target, float dt);


enum ViewType {
    viewType_graph2d,
    viewType_field,
    viewType_net,
    viewType_swerveDrive,
};
union ViewUnion {
    Graph2dInfo graph2dInfo;
    FieldInfo fieldInfo;
    NetInfo netInfo;
    SwerveDriveInfo swerveDriveInfo;

    ViewUnion() {  };
};
struct View {
    ViewUnion data;
    ViewType type;
    gfx_Framebuffer* target; // CLEANUP: bad
};



#define UI_VIEW_COUNT 3
static struct UIGlobs {

    gfx_Shader* sceneShader3d = nullptr;
    gfx_Shader* sceneShader2d = nullptr;
    gfx_Texture* solidTex;

    float rightSize;
    float downSize;

    View views[UI_VIEW_COUNT];

    gfx_Texture* wheelTex = nullptr;
    gfx_Texture* treadTex = nullptr;
    gfx_Texture* arrowTex = nullptr;

    gfx_VertexArray* robotVA = nullptr;
    gfx_IndexBuffer* robotIB = nullptr;
    gfx_VertexArray* fieldVA = nullptr;
    gfx_IndexBuffer* fieldIB = nullptr;

    gfx_Texture* fieldTex = nullptr;
} globs;



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

    // CLEANUP: there are like 6 different instances of this exact piece of code
    int w = (int)area->calculatedSizes[blu_axis_X];
    int h = (int)area->calculatedSizes[blu_axis_Y];
    if(w != target->texture->width || h != target->texture->height) {
        if(w > 0 && h > 0) {
            gfx_resizeFramebuffer(target, w, h);
        }
    }
}


void initView(View* v, BumpAlloc* resArena) {
    if(v->type == viewType_field) { v->data.fieldInfo = FieldInfo(); }
    else if(v->type == viewType_graph2d) { v->data.graph2dInfo = Graph2dInfo(); initGraph2dInfo(&v->data.graph2dInfo, resArena); }
    else if(v->type == viewType_swerveDrive) { v->data.swerveDriveInfo = SwerveDriveInfo(); }
    else if(v->type == viewType_net) { v->data.netInfo = NetInfo(); }
    else { ASSERT(false); };
}

void updateView(View* v, float dt, GLFWwindow* window, BumpAlloc* scratch) {
    if(v->type == viewType_field) { draw_field(&v->data.fieldInfo, v->target, dt, window); }
    else if(v->type == viewType_graph2d) { draw_graph2d(&v->data.graph2dInfo, v->target, dt, scratch); }
    else if(v->type == viewType_swerveDrive) { draw_swerveDrive(&v->data.swerveDriveInfo, v->target, dt); }
    else if(v->type == viewType_net) { draw_network(&v->data.netInfo, dt, scratch); }
    else { ASSERT(false); };
}


blu_Style borderStyle;


void ui_init(BumpAlloc* frameArena, BumpAlloc* resArena, gfx_Texture* solidTex) {

    globs.rightSize = 400;
    globs.downSize = 400;

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
    globs.views[1].type = viewType_graph2d;
    globs.views[2].type = viewType_net;

    initView(&globs.views[0], resArena);
    initView(&globs.views[1], resArena);
    initView(&globs.views[2], resArena);


    bool res = gfx_loadOBJMesh("res/models/Chassis2.obj", frameArena, &globs.robotVA, &globs.robotIB);
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



    globs.sceneShader2d = gfx_registerShader(gfx_vtype_POS2F_UV, "res/shaders/2d.vert", "res/shaders/2d.frag", frameArena);

    globs.sceneShader2d->passUniformBindFunc = [](gfx_Pass* pass, gfx_UniformBlock* uniforms) {
        int loc = glGetUniformLocation(pass->shader->id, "uVP");
        glUniformMatrix4fv(loc, 1, false, &(uniforms->vp)[0]);

        gfx_bindVertexArray(pass, gfx_getQuadVA());
        gfx_bindIndexBuffer(pass, gfx_getQuadIB());
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
    };
}







void draw_swerveDrive(SwerveDriveInfo* info, gfx_Framebuffer* target, float dt) {

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

    // TODO: hash get is not typesafe
    net_Prop* props[] = {
        net_hashGet(STR("FLSteerPos")),
        net_hashGet(STR("FRSteerPos")),
        net_hashGet(STR("BLSteerPos")),
        net_hashGet(STR("BRSteerPos"))
    };

    net_Prop* angle = net_hashGet(STR("yaw"));

    Mat4f temp;


    for(int i = 0; i < 4; i++) {
        gfx_UniformBlock* b = gfx_registerCall(p);
        b->texture = globs.wheelTex;

        matrixTranslation(translations[i].x, translations[i].y, 0, b->model);
        matrixScale(1, 1, 0, temp);
        b->model = b->model * temp;

        if(angle) {
            matrixZRotation(-(F32)angle->data->f64, temp);
            b->model = b->model * temp;
        }

        if(props[i]) {
            matrixZRotation(-(F32)props[i]->data->f64 * 360, temp);
            b->model = temp * b->model;
        }
    }

    gfx_UniformBlock* b = gfx_registerCall(p);
    b->texture = globs.arrowTex;
    if(angle) {
        matrixZRotation(-(F32)angle->data->f64, b->model); }

    matrixScale((F32)globs.arrowTex->width / globs.arrowTex->height, 1, 1, temp);
    b->model = temp * b->model;
}






void draw_line(gfx_Pass* p, float thickness, V4f color, V2f start, V2f end) {
    gfx_UniformBlock* b = gfx_registerCall(p);
    b->texture = globs.solidTex;
    b->color = color;

    Transform t;

    V2f center = (start+end) / 2;
    t.x = center.x;
    t.y = center.y;
    t.sx = (end-start).length();
    t.sy = 2;
    t.rz = -v2fAngle(end-start);
    b->model = matrixTransform(t);
}

void draw_graph2d(Graph2dInfo* info, gfx_Framebuffer* target, float dt, BumpAlloc* scratch) {

    // APPEND NEW VALUES
    for(int i = 0; i < GRAPH2D_LINECOUNT; i++) {
        if(!info->keys[i].chars) { continue; }

        net_Prop* prop = net_hashGet(info->keys[i]);

        bool nConn = net_getConnected();
        float nval = 0;
        if(prop) { nval = (F32)prop->data->f64; }
        else{ nConn = false; }

        // CLEANUP: profiling
        memmove(info->vals[i], &(info->vals[i][1]), (GRAPH2D_VCOUNT-1)*sizeof(float));
        info->vals[i][GRAPH2D_VCOUNT - 1] = nval;

        memmove(info->connectionVals[i], &(info->connectionVals[i][1]), (GRAPH2D_VCOUNT-1)*sizeof(U8));
        info->connectionVals[i][GRAPH2D_VCOUNT - 1] = nConn;
    }



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
        p->shader = globs.sceneShader2d;
        matrixOrtho(0, width, height, 0, 0, 100, p->passUniforms.vp);


        blu_WidgetInteraction inter = blu_interactionFromWidget(a);
        info->yScale += -inter.scrollDelta * 10;
        info->yScale = max(0, info->yScale);
        info->yOffset += inter.dragDelta.y;

        float scale = -info->yScale;
        float offset = info->yOffset + height/2;
        float pointGap = width / (float)GRAPH2D_VCOUNT;

        // TODO: batch line calls
        draw_line(p, 1, col_darkGray, { 0, offset }, { width, offset });
        draw_line(p, 1, col_darkGray, { 0, offset + 1*scale}, { width, offset + 1*scale });
        draw_line(p, 1, col_darkGray, { 0, offset - 1*scale}, { width, offset - 1*scale });



        for(int i = 0; i < GRAPH2D_LINECOUNT; i++) {
            if(!info->keys[i].chars) { continue; }

            V2f lastPoint = V2f(0, info->vals[i][0] * scale + offset);
            for(int j = 1; j < GRAPH2D_VCOUNT; j++) {

                V4f color = info->colors[i];
                if(!(info->connectionVals[i][j])) { color *= col_disconnect; }

                V2f point = { j*pointGap, info->vals[i][j] * scale + offset };
                draw_line(p, 2, color, lastPoint, point);
                lastPoint = point;
            }
        }


        a = blu_areaMake("lowerBit", blu_areaFlags_DRAW_BACKGROUND);
        blu_style_backgroundColor(col_darkGray, &a->style);
        blu_style_sizeY({ blu_sizeKind_TEXT, 0 }, &a->style);
        blu_style_childLayoutAxis(blu_axis_X, &a->style);
        blu_parentScope(a) {
            blu_styleScope(blu_Style()) {
            blu_style_sizeY({ blu_sizeKind_TEXT, 0 });
            blu_style_sizeX({ blu_sizeKind_TEXT, 0 });
            blu_style_backgroundColor(col_lightGray);
            blu_style_cornerRadius(5);

                for(int i = 0; i < GRAPH2D_LINECOUNT; i++) {
                    if(!info->keys[i].chars) { continue; }

                    a = blu_areaMake(str_format(scratch, STR("thing %i"), i),
                        blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DRAW_TEXT |
                        blu_areaFlags_DROP_EVENTS | blu_areaFlags_HOVER_ANIM);

                    blu_areaAddDisplayStr(a, info->keys[i]);

                    a->style.backgroundColor = v4f_lerp(a->style.backgroundColor, col_white, a->target_hoverAnim);

                    a->dropTypeMask = dropMasks_NT_PROP;
                    inter = blu_interactionFromWidget(a);
                    if(inter.dropped) {
                        net_Prop* prop = ((net_Prop*)(inter.dropVal));
                        if(prop->type == net_propType_F64) {
                            info->keys[i] = prop->name;
                        }
                    }
                }
            }
        }
    } // end area
};



// TODO: make controls not apply unless field is "selected"
// TODO: robot follow mode

blu_WidgetInteraction makeButton(str text, V4f hoverColor) {
    // TODO: button textures
    blu_Area* a = blu_areaMake(text, blu_areaFlags_CLICKABLE | blu_areaFlags_CENTER_TEXT | blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DRAW_TEXT | blu_areaFlags_HOVER_ANIM);
    a->style.backgroundColor = v4f_lerp(a->style.backgroundColor, hoverColor, a->target_hoverAnim);
    blu_areaAddDisplayStr(a, text);
    return blu_interactionFromWidget(a);
}

void draw_field(FieldInfo* info, gfx_Framebuffer* fb, float dt, GLFWwindow* window) {

    V4f mVec = { 0, 0, 0, 0 };

    F32 moveSpeed = 3;
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



    net_Prop* posX = net_hashGet(STR("posX"));
    net_Prop* posY = net_hashGet(STR("posY"));
    net_Prop* yaw = net_hashGet(STR("yaw"));

    net_Prop* estX = net_hashGet(STR("estX"));
    net_Prop* estY = net_hashGet(STR("estY"));

    Transform robotTransform = Transform();
    if(posX && posY && yaw) {
        robotTransform.x = (F32)posX->data->f64;
        robotTransform.z = -(F32)posY->data->f64;
        robotTransform.ry = -(F32)yaw->data->f64;
    }

    Transform estimateTransform = Transform();
    if(estX && estY && yaw) {

        Transform t = Transform();
        estimateTransform.x = (F32)estX->data->f64;
        estimateTransform.z = -(F32)estY->data->f64;
        estimateTransform.ry = -(F32)yaw->data->f64;
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









void draw_network(NetInfo* info, float dt, BumpAlloc* scratch) {
    blu_Area* a;


    blu_styleScope(blu_Style()) {
    blu_style_sizeY({ blu_sizeKind_TEXT, 0 });
    blu_style_childLayoutAxis(blu_axis_X);
    blu_style_backgroundColor(col_darkGray);

        a = blu_areaMake(STR("FPS"), blu_areaFlags_DRAW_TEXT | blu_areaFlags_DRAW_BACKGROUND);
        str n = str_format(scratch, STR("FPS: %f"), 1/dt);
        blu_areaAddDisplayStr(a, n);

        a = blu_areaMake(STR("connectionStatus"), blu_areaFlags_DRAW_BACKGROUND);
        blu_parentScope(a) {
            a = blu_areaMake(STR("label"), blu_areaFlags_DRAW_TEXT);
            blu_areaAddDisplayStr(a, STR("Network status: "));
            blu_style_sizeX({ blu_sizeKind_TEXT, 0 }, &a->style);

            a = blu_areaMake(STR("box"), blu_areaFlags_DRAW_BACKGROUND);
            a->style.backgroundColor = col_red;
            a->style.sizes[blu_axis_X] = { blu_sizeKind_REMAINDER, 0 };
            a->style.cornerRadius = 2;
            if(net_getConnected()) {
                a->style.backgroundColor = col_green; }
        } // end connection parent
    } // end top bar section



    a = blu_areaMake(STR("lower"), blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_CLICKABLE);
    info->clipSize = a->calculatedSizes[blu_axis_Y];
    a->style.childLayoutAxis = blu_axis_X;
    blu_parentScope(a) {

        a = blu_areaMake(STR("clip"), blu_areaFlags_VIEW_OFFSET | blu_areaFlags_CLICKABLE | blu_areaFlags_SCROLLABLE);
        blu_Area* clip = a;
        a->style.sizes[blu_axis_X] = { blu_sizeKind_REMAINDER, 0 };
        a->style.childLayoutAxis = blu_axis_Y;
        info->clipPos += blu_interactionFromWidget(a).scrollDelta * 40;
        blu_parentScope(a) {

            blu_styleScope(blu_Style()) {
            blu_style_sizeY({ blu_sizeKind_TEXT, 0 });
            blu_style_childLayoutAxis(blu_axis_X);
            blu_style_backgroundColor(col_darkGray);



                net_Prop** tracked;
                U32 tCount = 0;
                net_getTracked(&tracked, &tCount);
                for(int i = 0; i < tCount; i++) {
                    net_Prop* prop = tracked[i];
                    str quotedName = str_format(scratch, STR("\"%s\""), prop->name);

                    blu_Area* parent = blu_areaMake(prop->name, blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_HOVER_ANIM | blu_areaFlags_CLICKABLE);
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

                            a = blu_areaMake(STR("label"), blu_areaFlags_DRAW_TEXT);
                            blu_areaAddDisplayStr(a, quotedName);

                            if(!net_getConnected()) {
                                a->style.backgroundColor *= col_disconnect;
                                a->style.textColor *= col_disconnect;
                            }

                            a = blu_areaMake(STR("value"), blu_areaFlags_DRAW_TEXT | blu_areaFlags_DRAW_BACKGROUND);
                            a->style.backgroundColor = col_darkGray;


                            if(prop->type == net_propType_S32) {
                                blu_areaAddDisplayStr(a, str_format(scratch, STR("%i"), (prop->data->s32))); }
                            else if(prop->type == net_propType_F64) {
                                blu_areaAddDisplayStr(a, str_format(scratch, STR("%f"), (prop->data->f64))); }
                            else if(prop->type == net_propType_STR) {
                                blu_areaAddDisplayStr(a, str_format(scratch, STR("\"%s\""), (prop->data->str))); }
                            else if(prop->type == net_propType_BOOL) {
                                blu_areaAddDisplayStr(a, str_format(scratch, STR("%b"), (prop->data->boo)));
                                a->style.backgroundColor = prop->data->boo? col_green : col_red;
                                a->style.cornerRadius = 2;
                                a->style.textColor = col_darkBlue;
                            }

                            if(!net_getConnected()) {
                                a->style.backgroundColor *= col_disconnect;
                                a->style.textColor *= col_disconnect;
                            }
                        }
                    }
                }

            } // end text styling

        } // end of clip

        blu_Area* spacer = nullptr;
        if(info->clipMax > info->clipSize) {
            a = blu_areaMake(STR("scrollArea"), blu_areaFlags_DRAW_BACKGROUND);
            a->style.backgroundColor = col_darkGray;
            a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 10 };
            a->style.childLayoutAxis = blu_axis_Y;

            blu_parentScope(a) {

                a = blu_areaMake(STR("spacer"), 0);
                spacer = a;

                a = blu_areaMake(STR("bar"),
                    blu_areaFlags_DRAW_BACKGROUND |
                    blu_areaFlags_CLICKABLE);
                a->style.backgroundColor = col_lightGray;
                a->style.sizes[blu_axis_Y] = { blu_sizeKind_PERCENT, info->clipSize / info->clipMax };
                info->clipPos += blu_interactionFromWidget(a).dragDelta.y / info->clipSize * info->clipMax;

            }

            info->clipPos = max(info->clipPos, 0);
            info->clipPos = min(info->clipPos, info->clipMax - (info->clipSize));
            spacer->style.sizes[blu_axis_Y] = { blu_sizeKind_PX, (info->clipPos / info->clipMax) * info->clipSize };
        } else {
            info->clipPos = 0;
        }

        clip->viewOffset = { 0, info->clipPos };
    } // end left
}






// TODO: button texures
void makeViewSrc(const char* name, ViewType type) {

    blu_Area* a = blu_areaMake(name,
        blu_areaFlags_HOVER_ANIM | blu_areaFlags_DRAW_TEXT | blu_areaFlags_CLICKABLE | blu_areaFlags_DRAW_BACKGROUND);
    blu_areaAddDisplayStr(a, name);
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

void makeView(const char* name, View* v, float dt, BumpAlloc* scratch, GLFWwindow* window, BumpAlloc* res) {

    blu_Area* a = blu_areaMake(name, blu_areaFlags_DROP_EVENTS | blu_areaFlags_HOVER_ANIM);
    blu_style_childLayoutAxis(blu_axis_Y, &a->style);
    a->dropTypeMask = dropMasks_VIEW_TYPE;

    blu_WidgetInteraction inter = blu_interactionFromWidget(a);
    if(inter.dropped) {
        v->type = (ViewType)(U64)inter.dropVal; // sorry
        initView(v, res);
    }

    float t = a->target_hoverAnim;


    blu_parentScope(a) {
        blu_styleScope(blu_Style()) {
        blu_style_sizeX({ blu_sizeKind_PERCENT, 1 });
        blu_style_sizeY({ blu_sizeKind_PERCENT, 1 });

            updateView(v, dt, window, scratch);

            a = blu_areaMake("hoverIndicator", blu_areaFlags_FLOATING | blu_areaFlags_DRAW_BACKGROUND);
            a->style.backgroundColor = v4f_lerp(V4f(0, 0, 0, 0), V4f(1, 1, 1, 0.3), t);
        }
    }
}

void ui_update(BumpAlloc* scratch, BumpAlloc* res, GLFWwindow* window, float dt) {

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
            }
        }

        a = blu_areaMake(STR("leftBarSep"), blu_areaFlags_DRAW_BACKGROUND);
        a->style.backgroundColor = col_black;
        a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 3 };


        blu_styleScope(blu_Style())
        {
        blu_style_sizeX({blu_sizeKind_REMAINDER, 0 });
            makeView("left", &globs.views[0], dt, scratch, window, res);
        }


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

                makeView("top", &globs.views[1], dt, scratch, window, res);
            }


            a = blu_areaMake("YresizeBar", blu_areaFlags_CLICKABLE | blu_areaFlags_HOVER_ANIM | blu_areaFlags_DRAW_BACKGROUND);
            a->style.sizes[blu_axis_Y] = { blu_sizeKind_PX, 3 };

            blu_WidgetInteraction inter = blu_interactionFromWidget(a);
            float t = inter.held? 1 : a->target_hoverAnim;
            a->style.backgroundColor = v4f_lerp(col_black, col_white, t);
            globs.downSize += -inter.dragDelta.y;
            a->cursor = blu_cursor_resizeV;

            blu_styleScope(blu_Style()) {
            blu_style_sizeY({blu_sizeKind_PX, globs.downSize });
                makeView("bottom", &globs.views[2], dt, scratch, window, res);
            }
        }
    }
}


