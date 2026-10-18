#pragma once

#include "framework/boot.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(__ANDROID__) || defined(ANDROID)
// SDL reports Android window and touch coordinates in physical pixels. Keep UI
// layout in density-independent points, like iOS, while rendering at full resolution.
inline bool Android_UpdateLayoutMetrics(vtx::VertexContext *ctx)
{
    if (!ctx || !ctx->sdlWindow)
        return false;

    int windowW = 0, windowH = 0;
    int drawableW = 0, drawableH = 0;
    SDL_GetWindowSize(ctx->sdlWindow, &windowW, &windowH);
    SDL_GL_GetDrawableSize(ctx->sdlWindow, &drawableW, &drawableH);
    if (windowW <= 0 || windowH <= 0 || drawableW <= 0 || drawableH <= 0)
        return false;

    static const float densityScale = [] {
        float densityDpi = 160.0f;
        if (SDL_GetDisplayDPI(0, &densityDpi, nullptr, nullptr) != 0 || densityDpi <= 0.0f)
            densityDpi = 160.0f;
        return densityDpi / 160.0f;
    }();
    const int layoutW = windowW / densityScale >= 1.0f
        ? static_cast<int>(windowW / densityScale + 0.5f) : 1;
    const int layoutH = windowH / densityScale >= 1.0f
        ? static_cast<int>(windowH / densityScale + 0.5f) : 1;
    const float framebufferScale = static_cast<float>(drawableW) / layoutW;
    const bool changed = ctx->screenWidth != layoutW || ctx->screenHeight != layoutH ||
        ctx->pixelRatio != framebufferScale;

    ctx->screenWidth = layoutW;
    ctx->screenHeight = layoutH;
    ctx->pixelRatio = framebufferScale;
    glViewport(0, 0, drawableW, drawableH);
    if (changed)
        SDL_Log("Android display: %dx%d pixels, %dx%d layout points, density %.2f, framebuffer scale %.2f",
                drawableW, drawableH, layoutW, layoutH, densityScale, framebufferScale);
    return changed;
}
#endif

void handleWindowResize(vtx::VertexContext *ctx, int width, int height)
{
#if defined(__ANDROID__) || defined(ANDROID)
    Android_UpdateLayoutMetrics(ctx);
    return;
#endif
    if (height == 0)
        height = 1; // Prevent division by zero in perspective matrices

#if TARGET_OS_IOS || TARGET_IPHONE_SIMULATOR
    int drawW = width;
    int drawH = height;
    if (ctx && ctx->sdlWindow)
        SDL_GL_GetDrawableSize(ctx->sdlWindow, &drawW, &drawH);
    glViewport(0, 0, drawW, drawH);
#elif TARGET_OS_MAC
    glViewport(0, 0, width * ctx->pixelRatio, height * ctx->pixelRatio);
#else
    glViewport(0, 0, width, height);
#endif
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

bool handle_resize_sdl(vtx::VertexContext *ctx, SDL_Event event)
{
    if (event.type == SDL_WINDOWEVENT)
    {
        if (event.window.event == SDL_WINDOWEVENT_CLOSE)
        {
            if (ctx)
                ctx->shouldContinue = false;
            return false;
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

            // Only need this to be done in emscripten,
            // because in native it already calls it when window is manipulated
            SDL_SetWindowSize(ctx->sdlWindow, newWidth, newHeight);
        }
#else
        if (
            event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
#if TARGET_OS_IOS || TARGET_IPHONE_SIMULATOR
            SDL_GetWindowSize(ctx->sdlWindow, &newWidth, &newHeight);
#else
            newWidth = event.window.data1;
            newHeight = event.window.data2;
#endif
        }
        else
        {
            // Fallback if resize event not caught properly
            if (newWidth <= 0 && newHeight <= 0)
            {
#if TARGET_OS_IOS || TARGET_IPHONE_SIMULATOR
                SDL_GetWindowSize(ctx->sdlWindow, &newWidth, &newHeight);
#else
                SDL_GL_GetDrawableSize(ctx->sdlWindow, &newWidth, &newHeight);
#endif
            }
        }
#endif
        if (newWidth > 0 && newHeight > 0)
        {
            // should do this for all platforms
            handleWindowResize(ctx, newWidth, newHeight);

            return true; // was resized
        }
    }
    return false; // was not resized
}
