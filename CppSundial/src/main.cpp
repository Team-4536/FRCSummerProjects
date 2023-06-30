
#include <stdio.h>
#include <math.h>

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "stb_image/stb_image.h"
#include "stb_truetype/stb_truetype.h"
#include "GLAD/gl.h"

#include "base/arr.h"
#include "graphics.h"
#include "blue/blue.h"

#include "colors.h"




// NOTE: I didn't want to put it in gfx because of the stb dependency
gfx_Texture* loadTextureFromFile(const char* path) {

    // CLEANUP: image data is performing an allocaiton that I don't like
    stbi_set_flip_vertically_on_load(1); // NOTE: opengl specific
    S32 bpp, w, h;
    U8* data = stbi_load(path, &w, &h, &bpp, 4);
    ASSERT(data);

    return gfx_registerTexture(data, w, h, gfx_texPxType_RGBA8);
}



int main() {

    BumpAlloc lifetimeArena;
    BumpAlloc frameArena;
    bump_allocate(lifetimeArena, 1000000);
    bump_allocate(frameArena, 1000000);



    GLFWwindow* window = nullptr;
    GLFWcursor* hoverCursor = nullptr;
    // GLFW INIT AND WINDOW CREATION ===============================================================================
    {
        assert(glfwInit());
        glfwSetErrorCallback([](int error, const char* description) { printf("%s\n", description); });

        glfwWindowHint(GLFW_MAXIMIZED, true);
        glfwWindowHint(GLFW_RESIZABLE, true);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_SAMPLES, 1);

        window = glfwCreateWindow(512, 512, "Sundial", NULL, NULL);
        ASSERT(window);

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);


        hoverCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
        // glfwSetKeyCallback(window, updateInput);
        // glfwSetCursorPosCallback(window, updateMousePos);
        // glfwSetMouseButtonCallback(window, updateMouseButton);
    }

    // OPENGL/GLAD INIT
    {
        gladLoadGL(glfwGetProcAddress);
        glLoadIdentity();

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthFunc(GL_LESS | GL_EQUAL);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_MULTISAMPLE);

        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(
        [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
            printf("%s\n", message);
        }, 0);
    }

    gfx_init();



    gfx_Texture* solidTex = loadTextureFromFile("res/textures/solid.png");
    gfx_Texture* testTex = loadTextureFromFile("res/textures/helloTex.png");
    blu_init(solidTex);
    blu_loadFont("C:/windows/fonts/consola.ttf");





    gfx_Shader* blueShader;
    {
        blueShader = gfx_registerShader(gfx_vtype_POS2F_UV, "res/shaders/blue.vert", "res/shaders/blue.frag", frameArena);

        // TODO: remove lambdas
        blueShader->passUniformBindFunc = [](gfx_Shader* shader, gfx_UniformBlock* uniforms) {
            int loc;

            loc = glGetUniformLocation(shader->id, "uVP");
            glUniformMatrix4fv(loc, 1, false, &(uniforms->vp)[0]);
        };

        blueShader->uniformBindFunc = [](gfx_Shader* shader, gfx_UniformBlock* uniforms) {
            int loc;

            loc = glGetUniformLocation(shader->id, "uDstStart");
            glUniform2f(loc, uniforms->dstStart.x, uniforms->dstStart.y);
            loc = glGetUniformLocation(shader->id, "uDstEnd");
            glUniform2f(loc, uniforms->dstEnd.x, uniforms->dstEnd.y);

            loc = glGetUniformLocation(shader->id, "uSrcStart");
            glUniform2f(loc, uniforms->srcStart.x, uniforms->srcStart.y);
            loc = glGetUniformLocation(shader->id, "uSrcEnd");
            glUniform2f(loc, uniforms->srcEnd.x, uniforms->srcEnd.y);

            loc = glGetUniformLocation(shader->id, "uColor");
            glUniform4f(loc, uniforms->color.x, uniforms->color.y, uniforms->color.z, uniforms->color.w);

            loc = glGetUniformLocation(shader->id, "uTexture");
            glUniform1i(loc, 0);
            glActiveTexture(GL_TEXTURE0 + 0);
            glBindTexture(GL_TEXTURE_2D, uniforms->texture->id);

            loc = glGetUniformLocation(shader->id, "uFontTexture");
            glUniform1i(loc, 1);
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, uniforms->fontTexture->id);

            // CLEANUP: better texture abstraction
        };
    }




    V2f windowPos = V2f(400, 100);


    F64 prevTime = glfwGetTime();
    while(!glfwWindowShouldClose(window)) {
        F64 time = glfwGetTime();
        F64 dt = time - prevTime;
        prevTime = time;



        // UI ///////////////////////////////////////////////////////////////////////
        {
            F64 mx, my;
            glfwGetCursorPos(window, &mx, &my);
            bool leftPressed = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)? true : false;

            blu_beginFrame();
            blu_input(V2f((F32)mx, (F32)my), leftPressed);


            blu_Area* a;


            blu_styleScope
            {
            blu_style_add_backgroundColor(col_darkBlue);
            blu_style_add_textColor(col_white);
            blu_style_add_sizeX({ blu_sizeKind_PERCENT, 1 });
            blu_style_add_sizeY({ blu_sizeKind_PERCENT, 1 });
            blu_style_add_textPadding(V2f(3, 3));
            blu_style_add_animationStrength(0.1f);



                a = blu_areaMake(STR("left"), blu_areaFlags_DRAW_BACKGROUND);
                a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 200 };
                blu_parentScope(a) {
                    blu_styleScope {
                    blu_style_add_sizeX({ blu_sizeKind_TEXT, 0 });
                    blu_style_add_sizeY({ blu_sizeKind_TEXT, 0 });
                    blu_style_add_backgroundColor(col_darkGray);

                        a = blu_areaMake(STR("FPS"), blu_areaFlags_DRAW_TEXT);
                        char* buf = BUMP_PUSH_ARR(frameArena, 32, char);
                        gcvt(1/dt, 6, buf);
                        blu_areaAddDisplayStr(a, str_join(STR("FPS: "), STR(buf), frameArena));
                    }
                }


                a = blu_areaMake(STR("window"),
                        blu_areaFlags_DRAW_BACKGROUND |
                        blu_areaFlags_DRAW_TEXT |
                        blu_areaFlags_FLOATING);

                windowPos += blu_inputFromWidget(a).dragDelta;
                a->offset = windowPos;
                a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 350 };
                a->style.sizes[blu_axis_Y] = { blu_sizeKind_PX, 250 };
                a->style.childLayoutAxis = blu_axis_Y;

                blu_parentScope(a) {
                    blu_styleScope {
                    blu_style_add_sizeX({ blu_sizeKind_TEXT, 0 });
                    blu_style_add_sizeY({ blu_sizeKind_TEXT, 0 });
                    blu_style_add_backgroundColor(col_darkGray);

                        a = blu_areaMake(STR("titlebar"), blu_areaFlags_DRAW_BACKGROUND);
                        a->style.sizes[blu_axis_X] = { blu_sizeKind_PERCENT, 1 };
                        blu_parentScope(a) {

                            a = blu_areaMake(STR("title"), blu_areaFlags_DRAW_TEXT);
                            blu_areaAddDisplayStr(a, STR("Hello window!"));

                            a = blu_areaMake(STR("space"), 0);
                            a->style.sizes[blu_axis_X].kind = blu_sizeKind_REMAINDER;


                            blu_styleScope {
                                U32 stdSize = 30;
                                U32 wideSize = 100;

                                a = blu_areaMake(STR("b1"), blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DRAW_TEXT | blu_areaFlags_HOVER_ANIM);
                                blu_areaAddDisplayStr(a, STR("-"));
                                a->style.backgroundColor = v4f_lerp(a->style.backgroundColor, col_white, a->target_hoverAnim);
                                a->style.textColor = v4f_lerp(a->style.textColor, col_darkBlue, a->target_hoverAnim);
                                a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, lerp(stdSize, wideSize, a->target_hoverAnim) };

                                a = blu_areaMake(STR("b2"), blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DRAW_TEXT | blu_areaFlags_HOVER_ANIM);
                                blu_areaAddDisplayStr(a, STR("O"));
                                a->style.backgroundColor = v4f_lerp(a->style.backgroundColor, col_white, a->target_hoverAnim);
                                a->style.textColor = v4f_lerp(a->style.textColor, col_darkBlue, a->target_hoverAnim);
                                a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, lerp(stdSize, wideSize, a->target_hoverAnim) };

                                a = blu_areaMake(STR("b3"), blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DRAW_TEXT | blu_areaFlags_HOVER_ANIM);
                                blu_areaAddDisplayStr(a, STR("X"));
                                a->style.backgroundColor = v4f_lerp(a->style.backgroundColor, col_red, a->target_hoverAnim);
                                a->style.textColor = v4f_lerp(a->style.textColor, col_darkBlue, a->target_hoverAnim);
                                a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, lerp(stdSize, wideSize, a->target_hoverAnim) };
                            }
                        }


                    }
                }


            } // end main style
        } // end UI




        // RENDER UI //////////////////////////////////////////////////////////////////////////
        {
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            Mat4f vp;
            matrixOrtho(0, w, h, 0, 0.0001, 10000, vp);

            glViewport(0, 0, w, h);
            blu_layout(V2f(w, h));

            gfx_Pass* p = gfx_registerPass();
            p->shader = blueShader;
            p->passUniforms = gfx_UniformBlock();
            p->passUniforms.vp = vp;

            blu_createPass(p);
        }

        gfx_clear(V4f(0, 0, 0, 1));
        gfx_drawPasses();

        bump_clear(frameArena);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glfwDestroyWindow(window);
    glfwTerminate();

    // getchar();
}
