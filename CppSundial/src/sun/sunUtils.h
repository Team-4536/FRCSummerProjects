#pragma once
#include "base/str.h"
#include "base/utils.h"
#include "blue/blue.h"

// contains a string and a bunch of chars
struct NTKey {
    str str = { nullptr, 0 };
    U8 chars[255] = { 0 };
};
struct LineVert {
    V4f pos;
    V4f color;
};

enum DropMasks {
    dropMasks_NONE = 0,
    dropMasks_NT_PROP = (1 << 0),
    dropMasks_VIEW_TYPE = (1 << 1),
};

// adds a framebuffer to an area, and resizes based on area size
// doesnt rezise if area is <= 0 on either axis
void sun_areaAddFB(blu_Area* area, gfx_Framebuffer* target);

#define SUN_SCROLL_SENSITIVITY 40
#define SUN_SCROLL_BAR_SIZE 10
// returns parent to clip area that elems can be added
// NOTE: any styling cant be applied in post because it returns a child area, so put sizing in a scope
blu_Area* sun_makeScrollArea(float* pos);

blu_WidgetInteraction sun_makeButton(str text, V4f backColor, V4f hoverColor);
blu_WidgetInteraction sun_makeButton(str text, V4f hoverColor);

#ifdef SUN_IMPL
#include "colors.h"

void sun_areaAddFB(blu_Area* area, gfx_Framebuffer* target) {
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

blu_Area* sun_makeScrollArea(float* pos) {
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
        *pos += blu_interactionFromWidget(a).scrollDelta * SUN_SCROLL_SENSITIVITY;
        clipMax = a->calculatedSizes[blu_axis_Y];

        blu_Area* spacer = nullptr;
        if(clipMax > clipSize) {
            a = blu_areaMake("scrollArea", blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_SCROLLABLE);
            a->style.backgroundColor = col_darkGray;
            a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, SUN_SCROLL_BAR_SIZE };
            a->style.sizes[blu_axis_Y] = { blu_sizeKind_PERCENT, 1 };
            a->style.childLayoutAxis = blu_axis_Y;
            *pos += blu_interactionFromWidget(a).scrollDelta * SUN_SCROLL_SENSITIVITY;

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

blu_WidgetInteraction sun_makeButton(str text, V4f backColor, V4f hoverColor) {
    // TODO: button textures
    blu_Area* a = blu_areaMake(text, blu_areaFlags_CLICKABLE | blu_areaFlags_CENTER_TEXT | blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DRAW_TEXT | blu_areaFlags_HOVER_ANIM);
    a->style.backgroundColor = v4f_lerp(backColor, hoverColor, a->target_hoverAnim);
    blu_areaAddDisplayStr(a, text);
    return blu_interactionFromWidget(a);
}
blu_WidgetInteraction sun_makeButton(str text, V4f hoverColor) {
    blu_Area* a = blu_areaMake(text, blu_areaFlags_CLICKABLE | blu_areaFlags_CENTER_TEXT | blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DRAW_TEXT | blu_areaFlags_HOVER_ANIM);
    a->style.backgroundColor = v4f_lerp(a->style.backgroundColor, hoverColor, a->target_hoverAnim);
    blu_areaAddDisplayStr(a, text);
    return blu_interactionFromWidget(a);
}

#endif