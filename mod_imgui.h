#pragma once

#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "framework/boot.h"

struct ModImgui
{
    ImGuiContext* imGuiCtx;

    ImFont *font = nullptr;
    ImFont *custom_font = nullptr;

    void initImgui(vtx::VertexContext *ctx);
    void loadImgui(vtx::VertexContext *ctx);
    void hangImgui(vtx::VertexContext *ctx);

    void beginImgui();
    void endImgui();

    void processEvent(const SDL_Event *event, vtx::VertexContext *ctx) const;
    void newFrame() const;
    void renderFrame() const;
};

void ModImgui::hangImgui(vtx::VertexContext *ctx)
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void ModImgui::loadImgui(vtx::VertexContext *ctx)
{
    IMGUI_CHECKVERSION();
    ImGui::SetCurrentContext(nullptr);
    ImGui::CreateContext(); // new context entirely

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // as needed
    io.MouseDrawCursor = false;

    ImGui_ImplSDL2_InitForOpenGL(ctx->sdlWindow, ctx->sdlContext);

    /* 
     * On Mac it just requires #version 330 core (even if using Angle),
     * but for Emscripten it requires ES.
     * Other platforms TBA 
     */
#if defined(__EMSCRIPTEN__)
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 300 es");
    // Looks like for Macos we need to set IMGUI to ES too
    // ImGui_ImplOpenGL3_Init("#version 330 core");
#endif
}

void ModImgui::beginImgui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void ModImgui::endImgui()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ModImgui::processEvent(const SDL_Event *event, vtx::VertexContext *ctx) const
{
    // Copy one event
    SDL_Event newEvent = *event;
    
    // if (event->type == SDL_MOUSEBUTTONUP || event->type == SDL_MOUSEBUTTONDOWN) {
    //     std::cerr << "modifying that event" << std::endl;
    //     // newEvent.button.x = event->button.x;
    //     newEvent.button.y = ctx->screenHeight + event->button.y / 2.0f;

    // }
    // if (event->type == SDL_MOUSEMOTION) {

    //     newEvent.motion.y = -ctx->screenHeight + event->motion.y / 2.0f;
    //     newEvent.motion.yrel = event->motion.yrel * 2.0f;
    // }
    ImGui_ImplSDL2_ProcessEvent(&(newEvent));
}
