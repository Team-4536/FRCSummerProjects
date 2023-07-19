
#include <stdio.h>
#include <math.h>

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "stb_image/stb_image.h"
#include "stb_truetype/stb_truetype.h"
#include "GLAD/gl.h"

#include "base/arr.h"
#include "base/utils.h"
#include "graphics.h"
#include "blue/blue.h"
#include "network/network.h"

#include "ui.h"



// NOTE: I didn't want to put it in gfx because of the stb dependency
gfx_Texture* loadTextureFromFile(const char* path) {

    // CLEANUP: image data is performing an allocaiton that I don't like
    stbi_set_flip_vertically_on_load(1); // NOTE: opengl specific
    S32 bpp, w, h;
    U8* data = stbi_load(path, &w, &h, &bpp, 4);
    ASSERT(data);

    return gfx_registerTexture(data, w, h, gfx_texPxType_RGBA8);
}


float windowScrollDelta = 0;
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    windowScrollDelta -= (F32)yoffset;
}


int main() {


    BumpAlloc lifetimeArena;
    BumpAlloc frameArena;
    bump_allocate(&lifetimeArena, 1000000);
    bump_allocate(&frameArena, 10000000);


    GLFWwindow* window = nullptr;
    GLFWcursor* typeCursor = nullptr;
    GLFWcursor* handCursor = nullptr;
    GLFWcursor* resizeHCursor = nullptr;
    GLFWcursor* resizeVCursor = nullptr;
    // GLFW INIT AND WINDOW CREATION ===============================================================================
    {
        assert(glfwInit());
        glfwSetErrorCallback([](int error, const char* description) { printf("[GLFW] %s\n", description); });

        glfwWindowHint(GLFW_MAXIMIZED, true);
        glfwWindowHint(GLFW_RESIZABLE, true);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_SAMPLES, 4);

        window = glfwCreateWindow(512, 512, "Sundial", NULL, NULL);
        ASSERT(window);

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);


        handCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
        typeCursor = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
        resizeHCursor = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
        resizeVCursor = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
        glfwSetScrollCallback(window, scroll_callback);
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
            printf("[GL] %s\n", message);
        }, 0);
    }

    gfx_init();



    gfx_Texture* solidTex = loadTextureFromFile("res/textures/solid.png");
    blu_init(solidTex);
    blu_loadFont("C:/windows/fonts/consola.ttf");

    ui_init(&frameArena, &lifetimeArena, solidTex);
    net_init();


    gfx_Shader* blueShader;
    {
        blueShader = gfx_registerShader(gfx_vtype_POS2F_UV, "res/shaders/blue.vert", "res/shaders/blue.frag", &frameArena);

        // TODO: remove lambdas
        blueShader->passUniformBindFunc = [](gfx_Pass* pass, gfx_UniformBlock* uniforms) {
            int loc;

            loc = glGetUniformLocation(pass->shader->id, "uVP");
            glUniformMatrix4fv(loc, 1, false, &(uniforms->vp)[0]);

            gfx_bindVertexArray(pass, gfx_getQuadVA());
            gfx_bindIndexBuffer(pass, gfx_getQuadIB());
        };

        blueShader->uniformBindFunc = [](gfx_Pass* pass, gfx_UniformBlock* uniforms) {
            int loc;

            loc = glGetUniformLocation(pass->shader->id, "uBorderColor");
            glUniform4f(loc, uniforms->borderColor.x, uniforms->borderColor.y, uniforms->borderColor.z, uniforms->borderColor.w);

            loc = glGetUniformLocation(pass->shader->id, "uBorderSize");
            glUniform1f(loc, uniforms->borderSize);

            loc = glGetUniformLocation(pass->shader->id, "uCornerRadius");
            glUniform1f(loc, uniforms->cornerRadius);

            loc = glGetUniformLocation(pass->shader->id, "uDstStart");
            glUniform2f(loc, uniforms->dstStart.x, uniforms->dstStart.y);
            loc = glGetUniformLocation(pass->shader->id, "uDstEnd");
            glUniform2f(loc, uniforms->dstEnd.x, uniforms->dstEnd.y);

            loc = glGetUniformLocation(pass->shader->id, "uSrcStart");
            glUniform2f(loc, uniforms->srcStart.x, uniforms->srcStart.y);
            loc = glGetUniformLocation(pass->shader->id, "uSrcEnd");
            glUniform2f(loc, uniforms->srcEnd.x, uniforms->srcEnd.y);

            loc = glGetUniformLocation(pass->shader->id, "uClipStart");
            glUniform2f(loc, uniforms->clipStart.x, uniforms->clipStart.y);
            loc = glGetUniformLocation(pass->shader->id, "uClipEnd");
            glUniform2f(loc, uniforms->clipEnd.x, uniforms->clipEnd.y);

            loc = glGetUniformLocation(pass->shader->id, "uColor");
            glUniform4f(loc, uniforms->color.x, uniforms->color.y, uniforms->color.z, uniforms->color.w);

            loc = glGetUniformLocation(pass->shader->id, "uTexture");
            glUniform1i(loc, 0);
            glActiveTexture(GL_TEXTURE0 + 0);
            glBindTexture(GL_TEXTURE_2D, uniforms->texture->id);

            loc = glGetUniformLocation(pass->shader->id, "uFontTexture");
            glUniform1i(loc, 1);
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, uniforms->fontTexture->id);
        };
    }


    F64 prevTime = glfwGetTime();
    while(!glfwWindowShouldClose(window)) {

        F64 time = glfwGetTime();
        F64 dt = time - prevTime;
        prevTime = time;

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);

        F64 mx, my;
        glfwGetCursorPos(window, &mx, &my);
        bool leftPressed = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)? true : false;

        blu_beginFrame();



        ui_update(&frameArena, window, dt);





        blu_layout(V2f(w, h));

        blu_Cursor c;
        blu_input(V2f((F32)mx, (F32)my), leftPressed, windowScrollDelta, &c);
        if(c == blu_cursor_norm) { glfwSetCursor(window, nullptr); }
        else if(c == blu_cursor_hand) { glfwSetCursor(window, handCursor); }
        else if(c == blu_cursor_resizeH) { glfwSetCursor(window, resizeHCursor); }
        else if(c == blu_cursor_resizeV) { glfwSetCursor(window, resizeVCursor); }
        else if(c == blu_cursor_type) { glfwSetCursor(window, typeCursor); }
        else { ASSERT(false); }

        { // BLU RENDERING
            Mat4f vp;
            matrixOrtho(0, w, h, 0, 0.0001, 10000, vp);

            gfx_Pass* clear = gfx_registerPass();
            clear->isClearPass = true;
            clear->passUniforms.color = V4f(0, 0, 0, 1);

            gfx_Pass* p = gfx_registerPass();
            p->shader = blueShader;
            p->passUniforms = gfx_UniformBlock();
            p->passUniforms.vp = vp;

            blu_makeDrawCalls(p);
        }

        gfx_drawPasses(w, h);

        net_update(&frameArena, (F32)time);

        bump_clear(&frameArena);

        windowScrollDelta = 0;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    net_cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();

    // getchar();
}
