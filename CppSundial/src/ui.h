#pragma once

#include "graphics.h"
#include "base/utils.h"
#include "blue/blue.h"
#include "GLAD/gl.h"
#include "GLFW/glfw3.h"
#include "network/network.h"
#include "colors.h"


static struct UIGlobs {

    gfx_Shader* sceneShader = nullptr;
    gfx_Framebuffer* sceneFB = nullptr;

    gfx_VertexArray* sceneVA = nullptr;
    gfx_IndexBuffer* sceneIB = nullptr;

    V4f camPos = { 0, 0, 2, 0 };
    Mat4f robotTransform = Mat4f(1);
    V2i viewPortSize = { 0, 0 };

    V2f windowPos = V2f(350, 100);

    float clipSize = 0;
    float clipPos = 0;
    float clipMax = 1000;
    // TODO: adaptive max size

} globs;



// TODO: make deps clearer

void ui_init(BumpAlloc* frameArena) {
    globs = UIGlobs();
    globs.sceneFB = gfx_registerFramebuffer();

    float vaData[] = {
        -.5, -.5, 0,        0, 0,
        -.5, 0.5, 0,        0, 0,
        0.5, 0.5, 0,        0, 0,
        0.5, -.5, 0,        0, 0
    };
    globs.sceneVA = gfx_registerVertexArray(gfx_vtype_POS3F_UV, vaData, sizeof(vaData), false);

    U32 ibData[] = {
        0, 1, 2,
        2, 3, 0 };
    globs.sceneIB = gfx_registerIndexBuffer(ibData, sizeof(ibData) / sizeof(U32));



    {
        globs.sceneShader = gfx_registerShader(gfx_vtype_POS3F_UV, "res/shaders/scene.vert", "res/shaders/scene.frag", frameArena);

        globs.sceneShader->passUniformBindFunc = [](gfx_Pass* pass, gfx_UniformBlock* uniforms) {
            int loc;

            loc = glGetUniformLocation(pass->shader->id, "uVP");
            glUniformMatrix4fv(loc, 1, false, &(uniforms->vp)[0]);
        };

        globs.sceneShader->uniformBindFunc = [](gfx_Pass* pass, gfx_UniformBlock* uniforms) {
            int loc;

            loc = glGetUniformLocation(pass->shader->id, "uColor");
            glUniform4f(loc, uniforms->color.x, uniforms->color.y, uniforms->color.z, uniforms->color.w);

            loc = glGetUniformLocation(pass->shader->id, "uModel");
            glUniformMatrix4fv(loc, 1, false, &(uniforms->model)[0]);

            gfx_bindVertexArray(pass, uniforms->va);
            gfx_bindIndexBuffer(pass, uniforms->ib);
        };
    }

}



enum Component {
    comp_field,
    comp_network,
    comp_status,

    comp_driveSwerve,
    comp_driveMech,
    comp_driveTank,
};





void draw_field(float dt, GLFWwindow* window) {

    F32 moveSpeed = 3;
    globs.camPos.x += (glfwGetKey(window, GLFW_KEY_D) - glfwGetKey(window, GLFW_KEY_A)) * moveSpeed * dt;
    globs.camPos.y += (glfwGetKey(window, GLFW_KEY_W) - glfwGetKey(window, GLFW_KEY_S)) * moveSpeed * dt;
    globs.camPos.z += (glfwGetKey(window, GLFW_KEY_Q) - glfwGetKey(window, GLFW_KEY_E)) * moveSpeed * dt;


    // TODO: make str literal macro instead of strlen call
    net_Prop* posX = net_hashGet(STR("PosX"));
    net_Prop* posY = net_hashGet(STR("PosY"));
    net_Prop* yaw = net_hashGet(STR("Yaw"));

    if(posX && posY && yaw) {
        matrixTransform(
            (F32)posX->data->f64,
            (F32)posY->data->f64,
            0,
            (F32)yaw->data->f64,
            globs.robotTransform);
    }



    blu_Area* a = blu_areaMake(STR("fbdisplay"), blu_areaFlags_DRAW_TEXTURE);
    a->texture = globs.sceneFB->texture;

    int w = (int)a->calculatedSizes[blu_axis_X];
    int h = (int)a->calculatedSizes[blu_axis_Y];

    if(w != globs.sceneFB->texture->width || h != globs.sceneFB->texture->height) {
        gfx_resizeFramebuffer(globs.sceneFB, w, h); }




    gfx_Pass* p = gfx_registerPass();
    p->isClearPass = true;
    p->target = globs.sceneFB;
    p->passUniforms.color = V4f(0, 0, 0, 1);



    Mat4f proj = Mat4f(1.0f);
    if(h != 0) {
        matrixPerspective(90, (float)w / h, 0.01, 1000000, proj); }

    Mat4f view;
    matrixTranslation(globs.camPos.x, globs.camPos.y, globs.camPos.z, view);
    matrixInverse(view, view);

    p = gfx_registerPass();
    p->target = globs.sceneFB;
    p->shader = globs.sceneShader;
    p->passUniforms.vp = view * proj;



    gfx_UniformBlock* b = gfx_registerCall(p);
    b->color = V4f(1, 1, 1, 1);
    b->ib = globs.sceneIB;
    b->va = globs.sceneVA;
    b->model = globs.robotTransform;
}






void draw_network(float dt, BumpAlloc* scratch) {
    blu_Area* a;

    a = blu_areaMake(STR("left"), blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_CLICKABLE);
    globs.clipSize = a->calculatedSizes[blu_axis_Y];
    blu_areaAddDisplayStr(a, STR("left"));
    a->style.childLayoutAxis = blu_axis_X;
    blu_parentScope(a) {

        a = blu_areaMake(STR("clip"), blu_areaFlags_VIEW_OFFSET | blu_areaFlags_CLICKABLE);
        blu_Area* clip = a;
        blu_areaAddDisplayStr(a, STR("blip"));
        a->style.sizes[blu_axis_X] = { blu_sizeKind_REMAINDER, 0 };
        a->style.childLayoutAxis = blu_axis_Y;
        globs.clipPos += blu_interactionFromWidget(a).scrollDelta * 40;
        blu_parentScope(a) {

            blu_styleScope {
            blu_style_add_sizeY({ blu_sizeKind_TEXT, 0 });
            blu_style_add_childLayoutAxis(blu_axis_X);
            blu_style_add_backgroundColor(col_darkGray);

                str n = str_format(scratch, STR("FPS: %f"), 1/dt);
                blu_areaAddDisplayStr(a, n);

                a = blu_areaMake(STR("connectionPar"), blu_areaFlags_DRAW_BACKGROUND);
                blu_areaAddDisplayStr(a, STR("conPar"));
                blu_parentScope(a) {
                    a = blu_areaMake(STR("networkConnLabel"), blu_areaFlags_DRAW_TEXT);
                    blu_areaAddDisplayStr(a, STR("Network status: "));
                    a->style.sizes[blu_axis_X] = { blu_sizeKind_TEXT, 0 };

                    a = blu_areaMake(STR("connSpacer"), 0);
                    blu_areaAddDisplayStr(a, STR("conspace"));
                    a->style.sizes[blu_axis_X] = { blu_sizeKind_REMAINDER, 0 };

                    a = blu_areaMake(STR("connection"), blu_areaFlags_DRAW_BACKGROUND);
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
                            a = blu_areaMake(str_join(prop->name, STR("label"), scratch), blu_areaFlags_DRAW_TEXT);

                            str n = str_format(scratch, STR("\"%s\""), prop->name);
                            blu_areaAddDisplayStr(a, n);

                            a = blu_areaMake(str_join(prop->name, STR("value"), scratch), blu_areaFlags_DRAW_TEXT);
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
        if(globs.clipMax > globs.clipSize) {
            a = blu_areaMake(STR("scrollpar"), blu_areaFlags_DRAW_BACKGROUND);
            a->style.backgroundColor = col_darkGray;
            a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 10 };
            a->style.childLayoutAxis = blu_axis_Y;

            blu_parentScope(a) {

                a = blu_areaMake(STR("scspace"), 0);
                spacer = a;

                a = blu_areaMake(STR("scroll"),
                    blu_areaFlags_DRAW_BACKGROUND |
                    blu_areaFlags_CLICKABLE);
                a->style.backgroundColor = col_lightGray;
                a->style.sizes[blu_axis_Y] = { blu_sizeKind_PERCENT, globs.clipSize / globs.clipMax };
                globs.clipPos += blu_interactionFromWidget(a).dragDelta.y / globs.clipSize * globs.clipMax;

            }

            globs.clipPos = max(globs.clipPos, 0);
            globs.clipPos = min(globs.clipPos, globs.clipMax - (globs.clipSize));
            spacer->style.sizes[blu_axis_Y] = { blu_sizeKind_PX, (globs.clipPos / globs.clipMax) * globs.clipSize };
        } else {
            globs.clipPos = 0;
        }

        clip->viewOffset = { 0, globs.clipPos };



    } // end left
}








void ui_update(BumpAlloc* scratch, GLFWwindow* window, float dt) {


    blu_styleScope
    {
    blu_style_add_backgroundColor(col_darkBlue);
    blu_style_add_textColor(col_white);
    blu_style_add_sizeX({ blu_sizeKind_PERCENT, 1 });
    blu_style_add_sizeY({ blu_sizeKind_PERCENT, 1 });
    blu_style_add_textPadding(V2f(3, 3));
    blu_style_add_animationStrength(0.1f);


        blu_styleScope
        {
        blu_style_add_sizeX({blu_sizeKind_REMAINDER, 0 });
            draw_field(dt, window);
        }

        blu_styleScope
        {
        blu_style_add_sizeX({blu_sizeKind_PX, 300 });
            draw_network(dt, scratch);
        }
    }
}


