#pragma once

#include "graphics.h"
#include "blue/blue.h"
#include "sun/views.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#define SUN_VIEW_COUNT 4
struct SunGlobs {

    gfx_Shader* sceneShader3d = nullptr;
    gfx_Shader* sceneShader2d = nullptr;
    gfx_Shader* lineShader = nullptr;

    float rightSize;
    float downSizeL;
    float downSizeR;

    View views[SUN_VIEW_COUNT];

    gfx_Texture* solidTex;
    gfx_Texture* wheelTex = nullptr;
    gfx_Texture* treadTex = nullptr;
    gfx_Texture* arrowTex = nullptr;
    gfx_Texture* fieldTex = nullptr;

    gfx_Texture* controlTex = nullptr;
    gfx_Texture* fieldSrcTex = nullptr;
    gfx_Texture* graphTex = nullptr;
    gfx_Texture* powerTex = nullptr;
    gfx_Texture* swerveTex = nullptr;

    gfx_VertexArray* robotVA = nullptr;
    gfx_IndexBuffer* robotIB = nullptr;
    gfx_VertexArray* fieldVA = nullptr;
    gfx_IndexBuffer* fieldIB = nullptr;
    gfx_VertexArray* quadVA = nullptr;
    gfx_IndexBuffer* quadIB = nullptr;

    gfx_VertexArray* emptyVA = nullptr;

    sun_ControlsInfo ctrlInfo;

    net_Table table;
    GLFWwindow* window;
    float dt;
    BumpAlloc* scratch;
    float curTime;

    blu_Style borderStyle;
    blu_Style textSizeStyle;
};

void sun_init(BumpAlloc* frameArena, BumpAlloc* replayArena, gfx_Texture* solidTex);
void sun_update(BumpAlloc* scratch, GLFWwindow* window, float dt, float curTime);

#ifdef SUN_IMPL

#include "GLAD/gl.h"
#include "stb_image/stb_image.h"
#include "network/sockets.h"
#include "colors.h"

extern SunGlobs globs;

gfx_Texture* loadImage(const char* path) {
    int w, h, bpp;
    U8* data = stbi_load(path, &w, &h, &bpp, 4);
    ASSERT(data);
    return gfx_registerTexture(data, w, h, gfx_texPxType_RGBA8);
}

void sun_init(BumpAlloc* frameArena, BumpAlloc* replayArena, gfx_Texture* solidTex) {

    globs.rightSize = 400;
    globs.downSizeL = 100;
    globs.downSizeR = 400;

    globs.solidTex = solidTex;

    int w, h, bpp;
    stbi_set_flip_vertically_on_load(1);
    U8* data;

    blu_style_borderColor(col_black, &globs.borderStyle);
    blu_style_borderSize(1, &globs.borderStyle);

    blu_style_sizeX({ blu_sizeKind_TEXT, 0 }, &globs.textSizeStyle);
    blu_style_sizeY({ blu_sizeKind_TEXT, 0 }, &globs.textSizeStyle);

    for(int i = 0; i < SUN_VIEW_COUNT; i++) {
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

    globs.ctrlInfo = sun_ControlsInfo();
    globs.ctrlInfo.replayArena = replayArena;

    bool res = gfx_loadOBJMesh("res/models/Chassis3.obj", frameArena, &globs.robotVA, &globs.robotIB);
    ASSERT(res);
    bump_clear(frameArena);

    res = gfx_loadOBJMesh("res/models/plane.obj", frameArena, &globs.fieldVA, &globs.fieldIB);
    ASSERT(res);
    bump_clear(frameArena);

    globs.fieldTex = loadImage("res/textures/field.png");
    globs.wheelTex = loadImage("res/textures/swerveWheel.png");
    globs.treadTex = loadImage("res/textures/swerveTread.png");
    globs.arrowTex = loadImage("res/textures/vector.png");

    globs.controlTex = loadImage("res/textures/control_64.png");
    globs.fieldSrcTex = loadImage("res/textures/field_64.png");
    globs.graphTex = loadImage("res/textures/graph_32.png");
    globs.powerTex = loadImage("res/textures/power_64.png");
    globs.swerveTex = loadImage("res/textures/swerve_64.png");




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

// TODO: angle visualizer
// TODO: vector visualizer

void sun_update(BumpAlloc* scratch, GLFWwindow* window, float dt, float curTime) {

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
                makeViewSrc("Field", globs.fieldSrcTex, viewType_field);
                makeViewSrc("Graph", globs.graphTex, viewType_graph2d);
                makeViewSrc("Swerve", globs.swerveTex, viewType_swerveDrive);
                makeViewSrc("NT", globs.solidTex, viewType_net);
                makeViewSrc("Power", globs.powerTex, viewType_powerIndicators);
                makeViewSrc("Ctrls", globs.controlTex, viewType_controls);
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
#endif