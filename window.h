#pragma once

#include "framework/boot.h"

void handleWindowResize(vtx::VertexContext *ctx, int width, int height)
{
    if (height == 0)
        height = 1; // Prevent division by zero in perspective matrices

    glViewport(0, 0, width, height);

    ctx->screenHeight = height;
    ctx->screenWidth = width;
}
#ifdef __EMSCRIPTEN__
EM_JS(float, get_canvas_parent_width, (), {
    var parent = document.getElementById('canvas').parentElement;
    if (!parent)
    {
        return 0.0;
    }
    const style = window.getComputedStyle(parent);
    return parent.clientWidth - parseFloat(style.paddingLeft) - parseFloat(style.paddingRight);
});

EM_JS(float, get_canvas_parent_height, (), {
    var parent = document.getElementById('canvas').parentElement;
    if (!parent)
    {
        return 0.0;
    }
    const style = window.getComputedStyle(parent);

    return parent.clientHeight - parseFloat(style.paddingTop) // take away padding
           - parseFloat(style.paddingBottom)
});
#endif

void handle_resize_sdl(vtx::VertexContext *ctx, SDL_Event event)
{
    if (event.type == SDL_WINDOWEVENT)
    {
        if (event.window.event == SDL_WINDOWEVENT_CLOSE)
        {
            // vtx::exitVortex();
            abort();
            return;
        }

        int newWidth = -1;
        int newHeight = -1;
#ifdef __EMSCRIPTEN__
        if (
            event.window.event == SDL_WINDOWEVENT_RESIZED         // Trhiggered whern externally resized
            || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED // trhiggered when changed by operson
        )
        {
            // No matter what are the screen size values,
            // only resize to fill the canvas parent element
            // when it is running in the browser
            float parentWidth = get_canvas_parent_width();
            float parentHeight = get_canvas_parent_height();
            newWidth = (int)parentWidth;
            newHeight = (int)parentHeight;
            std::cerr << "Emscriptenn resizing to screen dimensions " << newWidth << "x" << newHeight << std::endl;

            // Only need this to be done in emscripten,
            // because in native it already calls it when window is manipulated
            SDL_SetWindowSize(ctx->sdlWindow, newWidth, newHeight);
        }
#else
        if (
            event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
            newWidth = event.window.data1;
            newHeight = event.window.data2;
        }
        else
        {
            // Fallback if resize event not caught properly
            if (newWidth <= 0 && newHeight <= 0)
            {
                SDL_GL_GetDrawableSize(ctx->sdlWindow, &newWidth, &newHeight);
            }
        }
#endif
        if (newWidth > 0 && newHeight > 0)
        {
            // should do this for all platforms
            handleWindowResize(ctx, newWidth, newHeight);
            // Dublicated in on screen size change event but
            // This is not required the actuallScreenSize ised where
            // usr->joy.setScreenSize(ctx->screenWidth, ctx->screenHeight);
            // usr->atlas.setScreenSize(ctx->screenWidth, ctx->screenHeight);
        }
    }
}