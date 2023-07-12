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





struct FieldInfo {
    gfx_Framebuffer* fb = nullptr;

    gfx_VertexArray* va = nullptr;
    gfx_IndexBuffer* ib = nullptr;

    gfx_Texture* fieldTex = nullptr;

    Transform camTransform;
    Transform camTarget;
};

struct NetInfo {
    float clipSize = 0;
    float clipPos = 0;
    float clipMax = 1000;
    // TODO: adaptive max size
};

struct SwerveDriveInfo {
    gfx_Framebuffer* target = nullptr;
    gfx_Texture* wheelTex = nullptr;
    gfx_Texture* treadTex = nullptr;
    gfx_Texture* arrowTex = nullptr;
};



static struct UIGlobs {

    gfx_Shader* sceneShader3d = nullptr;
    gfx_Shader* sceneShader2d = nullptr;
    gfx_Texture* solidTex;

    float rightSize = 400;
    float botSize = 200;


    FieldInfo fieldInfo = FieldInfo();
    NetInfo netInfo = NetInfo();
    SwerveDriveInfo swerveInfo = SwerveDriveInfo();
} globs;




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



void ui_init(BumpAlloc* frameArena, gfx_Texture* solidTex) {
    globs = UIGlobs();

    globs.solidTex = solidTex;

    int w, h, bpp;
    stbi_set_flip_vertically_on_load(1);
    U8* data;



    globs.fieldInfo.fb = gfx_registerFramebuffer();

    float vaData[] = {
        -.5, -.5, 0,        0, 0,
        -.5, 0.5, 0,        0, 1,
        0.5, 0.5, 0,        1, 1,
        0.5, -.5, 0,        1, 0
    };
    globs.fieldInfo.va = gfx_registerVertexArray(gfx_vtype_POS3F_UV, vaData, sizeof(vaData), false);

    U32 ibData[] = {
        0, 1, 2,
        2, 3, 0 };
    globs.fieldInfo.ib = gfx_registerIndexBuffer(ibData, sizeof(ibData) / sizeof(U32));

    data = stbi_load("res/textures/field.png", &w, &h, &bpp, 4);
    ASSERT(data);
    globs.fieldInfo.fieldTex = gfx_registerTexture(data, w, h, gfx_texPxType_RGBA8);




    globs.swerveInfo.target = gfx_registerFramebuffer();

    data = stbi_load("res/textures/swerveWheel.png", &w, &h, &bpp, 4);
    ASSERT(data);
    globs.swerveInfo.wheelTex = gfx_registerTexture(data, w, h, gfx_texPxType_RGBA8);

    data = stbi_load("res/textures/swerveTread.png", &w, &h, &bpp, 4);
    ASSERT(data);
    globs.swerveInfo.treadTex = gfx_registerTexture(data, w, h, gfx_texPxType_RGBA8);

    data = stbi_load("res/textures/vector.png", &w, &h, &bpp, 4);
    ASSERT(data);
    globs.swerveInfo.arrowTex = gfx_registerTexture(data, w, h, gfx_texPxType_RGBA8);







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







void draw_swerveDrive(SwerveDriveInfo* info, float dt) {

    blu_Area* a = blu_areaMake(STR("swerveDisplay"), blu_areaFlags_DRAW_TEXTURE);
    a->texture = info->target->texture;

    areaAddFB(a, info->target);

    gfx_Pass* p = gfx_registerClearPass(col_darkBlue, info->target);

    p = gfx_registerPass();
    p->target = info->target;
    p->shader = globs.sceneShader2d;

    float aspect = ((float)info->target->texture->width/info->target->texture->height);
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
    net_Prop* props[] = {
        net_hashGet(STR("FLSteerPos")),
        net_hashGet(STR("FRSteerPos")),
        net_hashGet(STR("BLSteerPos")),
        net_hashGet(STR("BRSteerPos"))
    };

    net_Prop* angle = net_hashGet(STR("Yaw"));

    Mat4f temp;


    for(int i = 0; i < 4; i++) {
        gfx_UniformBlock* b = gfx_registerCall(p);
        b->texture = info->wheelTex;

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
    b->texture = info->arrowTex;
    if(angle) {
        matrixZRotation(-(F32)angle->data->f64, b->model); }

    matrixScale((F32)info->arrowTex->width / info->arrowTex->height, 1, 1, temp);
    b->model = temp * b->model;
}





#define GRAPH2D_VCOUNT 200
struct Graph2dInfo {

    float vals[GRAPH2D_VCOUNT] = { 0 };

    gfx_Framebuffer* target = nullptr;
};

void draw_graph2d(Graph2dInfo* info, float dt, float nval) {

    memmove(info->vals, info->vals+1, GRAPH2D_VCOUNT - 1);
    info->vals[GRAPH2D_VCOUNT - 1] = nval;

    blu_Area* a = blu_areaMake(STR("graph2d"), blu_areaFlags_DRAW_BACKGROUND);
    a->style.childLayoutAxis = blu_axis_Y;

    blu_parentScope(a) {
        blu_styleScope {
        blu_style_add_sizeY({blu_sizeKind_REMAINDER});

            a = blu_areaMake(STR("upperBit"), blu_areaFlags_DRAW_TEXTURE);
            areaAddFB(a, info->target);
            gfx_registerClearPass(col_darkBlue, info->target);

            gfx_Pass* p = gfx_registerPass();
            p->target = info->target;
            p->shader = globs.sceneShader2d;
            p->passUniforms.vp = Mat4f(1);




            a = blu_areaMake(STR("lowerBit"), 0);
        }

    }
};



// TODO: make controls not apply unless field is "selected"
// TODO: robot follow mode

void draw_field(FieldInfo* info, float dt, GLFWwindow* window) {

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



    net_Prop* posX = net_hashGet(STR("PosX"));
    net_Prop* posY = net_hashGet(STR("PosY"));
    net_Prop* yaw = net_hashGet(STR("Yaw"));

    net_Prop* estX = net_hashGet(STR("EstX"));
    net_Prop* estY = net_hashGet(STR("EstY"));

    Transform robotTransform = Transform();
    robotTransform.rx = -90; // CLEANUP/TODO: temp constants, until a model is made
    robotTransform.sx = 0.711f;
    robotTransform.sy = 0.711f;
    if(posX && posY && yaw) {
        robotTransform.x = (F32)posX->data->f64;
        robotTransform.z = -(F32)posY->data->f64;
        robotTransform.ry = -(F32)yaw->data->f64;
    }

    Transform estimateTransform = Transform();
    estimateTransform.rx = -90;
    estimateTransform.sx = 0.711f;
    estimateTransform.sy = 0.711f;
    if(estX && estY && yaw) {

        Transform t = Transform();
        estimateTransform.x = (F32)estX->data->f64;
        estimateTransform.z = -(F32)estY->data->f64;
        estimateTransform.ry = -(F32)yaw->data->f64;
    }




    blu_Area* a = blu_areaMake(STR("fieldDisplay"), blu_areaFlags_DRAW_TEXTURE | blu_areaFlags_CLICKABLE);
    a->texture = info->fb->texture;

    V2f delta = blu_interactionFromWidget(a).dragDelta;
    info->camTarget.ry -= delta.x * 0.5f;
    info->camTarget.rx -= delta.y * 0.5f;

    areaAddFB(a, info->fb);

    Mat4f proj = Mat4f(1.0f);
    matrixPerspective(90, (float)info->fb->texture->width / info->fb->texture->height, 0.01, 1000000, proj);


    blu_parentScope(a) {
        blu_styleScope {
        blu_style_add_sizeX({ blu_sizeKind_PX, 50 });
        blu_style_add_sizeY({ blu_sizeKind_PX, 50 });

            // TODO: button textures
            a = blu_areaMake(STR("homeButton"), blu_areaFlags_CLICKABLE | blu_areaFlags_CENTER_TEXT | blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DRAW_TEXT | blu_areaFlags_FLOATING | blu_areaFlags_HOVER_ANIM);
            a->offset = V2f(0, 0);
            a->cursor = blu_cursor_hand;
            V4f tra = V4f(1, 1, 1, 0.5f);
            a->style.backgroundColor = v4f_lerp(col_darkGray * tra, col_white * tra, a->target_hoverAnim);
            blu_areaAddDisplayStr(a, STR("H"));

            if(blu_interactionFromWidget(a).clicked) {
                // HOME TARGET (above the field)
                info->camTarget = {
                    0, 10, 0,
                    -90, 0, 0,
                    1, 1, 1
                    };
            }
        }
    }



    gfx_Pass* p = gfx_registerClearPass(col_darkBlue, info->fb);


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
    p->target = info->fb;
    p->shader = globs.sceneShader3d;
    p->passUniforms.vp = view * proj;


    gfx_UniformBlock* b;


    b = gfx_registerCall(p);
    b->color = V4f(1, 1, 1, 1);
    b->ib = info->ib;
    b->va = info->va;
    b->texture = info->fieldTex;

    Transform field = Transform(); // TODO/CLEANUP: get a field model
    field.rx = -90;
    field.y = -0.2f;
    field.sx = 16.4846;
    field.sy = 8.1026;
    b->model = matrixTransform(field);


    b = gfx_registerCall(p);
    b->color = V4f(1, 0, 0, 1);
    b->ib = info->ib;
    b->va = info->va;
    b->model = matrixTransform(robotTransform);
    b->texture = globs.solidTex;

    b = gfx_registerCall(p);
    b->color = V4f(1, 0, 0, 0.5);
    b->ib = info->ib;
    b->va = info->va;
    b->model = matrixTransform(estimateTransform);
    b->texture = globs.solidTex;



}









void draw_network(NetInfo* info, float dt, BumpAlloc* scratch) {
    blu_Area* a;

    a = blu_areaMake(STR("left"), blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_CLICKABLE);
    info->clipSize = a->calculatedSizes[blu_axis_Y];
    a->style.childLayoutAxis = blu_axis_X;
    blu_parentScope(a) {

        a = blu_areaMake(STR("clip"), blu_areaFlags_VIEW_OFFSET | blu_areaFlags_CLICKABLE);
        blu_Area* clip = a;
        a->style.sizes[blu_axis_X] = { blu_sizeKind_REMAINDER, 0 };
        a->style.childLayoutAxis = blu_axis_Y;
        info->clipPos += blu_interactionFromWidget(a).scrollDelta * 40;
        blu_parentScope(a) {

            blu_styleScope {
            blu_style_add_sizeY({ blu_sizeKind_TEXT, 0 });
            blu_style_add_childLayoutAxis(blu_axis_X);
            blu_style_add_backgroundColor(col_darkGray);

                a = blu_areaMake(STR("FPS"), blu_areaFlags_DRAW_TEXT);
                str n = str_format(scratch, STR("FPS: %f"), 1/dt);
                blu_areaAddDisplayStr(a, n);

                a = blu_areaMake(STR("connectionStatus"), blu_areaFlags_DRAW_BACKGROUND);
                blu_parentScope(a) {
                    a = blu_areaMake(STR("label"), blu_areaFlags_DRAW_TEXT);
                    blu_areaAddDisplayStr(a, STR("Network status: "));
                    a->style.sizes[blu_axis_X] = { blu_sizeKind_TEXT, 0 };

                    a = blu_areaMake(STR("spacer"), 0);
                    a->style.sizes[blu_axis_X] = { blu_sizeKind_REMAINDER, 0 };

                    a = blu_areaMake(STR("box"), blu_areaFlags_DRAW_BACKGROUND);
                    a->style.backgroundColor = col_red;
                    a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 100 };
                    if(net_getConnected()) {
                        a->style.backgroundColor = col_green; }
                } // end connection parent


                net_Prop** tracked;
                U32 tCount = 0;
                net_getTracked(&tracked, &tCount);
                for(int i = 0; i < tCount; i++) {
                    net_Prop* prop = tracked[i];

                    a = blu_areaMake(prop->name, 0);

                    blu_parentScope(a) {
                        blu_styleScope {
                        blu_style_add_sizeX({ blu_sizeKind_PERCENT, 0.5 });
                            a = blu_areaMake(STR("label"), blu_areaFlags_DRAW_TEXT);

                            str n = str_format(scratch, STR("\"%s\""), prop->name);
                            blu_areaAddDisplayStr(a, n);

                            a = blu_areaMake(STR("value"), blu_areaFlags_DRAW_TEXT);
                            a->style.backgroundColor = col_darkGray;
                            if(prop->type == net_propType_S32) {
                                blu_areaAddDisplayStr(a, str_format(scratch, STR("%i"), (prop->data->s32))); }
                            else if(prop->type == net_propType_F64) {
                                blu_areaAddDisplayStr(a, str_format(scratch, STR("%f"), (prop->data->f64))); }
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








void ui_update(BumpAlloc* scratch, GLFWwindow* window, float dt) {

    blu_Area* a;

    blu_styleScope
    {
    blu_style_add_backgroundColor(col_darkBlue);
    blu_style_add_textColor(col_white);
    blu_style_add_sizeX({ blu_sizeKind_PERCENT, 1 });
    blu_style_add_sizeY({ blu_sizeKind_PERCENT, 1 });
    blu_style_add_textPadding(V2f(3, 3));
    blu_style_add_animationStrength(0.1f);


        a = blu_areaMake(STR("leftBarParent"), blu_areaFlags_DRAW_BACKGROUND);
        a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 100 };
        a->style.childLayoutAxis = blu_axis_Y;

        a = blu_areaMake(STR("leftBarSep"), blu_areaFlags_DRAW_BACKGROUND);
        a->style.backgroundColor = col_black;
        a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 3 };


        blu_styleScope
        {
        blu_style_add_sizeX({blu_sizeKind_REMAINDER, 0 });
            draw_field(&globs.fieldInfo, dt, window);
        }


        a = blu_areaMake(STR("XresizeBar"), blu_areaFlags_CLICKABLE | blu_areaFlags_HOVER_ANIM | blu_areaFlags_DRAW_BACKGROUND);
        a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 3 };

        blu_WidgetInteraction inter = blu_interactionFromWidget(a);
        float t = inter.held? 1 : a->target_hoverAnim;
        a->style.backgroundColor = v4f_lerp(col_black, col_white, t);
        globs.rightSize += -inter.dragDelta.x;
        a->cursor = blu_cursor_resizeH;



        a = blu_areaMake(STR("rightParent"), 0);
        a->style.sizes[blu_axis_X] = {blu_sizeKind_PX, globs.rightSize };
        a->style.childLayoutAxis = blu_axis_Y;


        blu_parentScope(a) {

            blu_styleScope {
            blu_style_add_sizeY({ blu_sizeKind_REMAINDER, 0 });
                draw_swerveDrive(&globs.swerveInfo, dt);
            }


            a = blu_areaMake(STR("YresizeBar"), blu_areaFlags_CLICKABLE | blu_areaFlags_HOVER_ANIM | blu_areaFlags_DRAW_BACKGROUND);
            a->style.sizes[blu_axis_Y] = { blu_sizeKind_PX, 3 };

            blu_WidgetInteraction inter = blu_interactionFromWidget(a);
            float t = inter.held? 1 : a->target_hoverAnim;
            a->style.backgroundColor = v4f_lerp(col_black, col_white, t);
            globs.botSize += -inter.dragDelta.y;
            a->cursor = blu_cursor_resizeV;

            blu_styleScope {
            blu_style_add_sizeY({blu_sizeKind_PX, globs.botSize });
                draw_network(&globs.netInfo, dt, scratch);
            }
        }
    }
}


