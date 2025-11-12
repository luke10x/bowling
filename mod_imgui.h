#pragma once

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "framework/ctx.h"

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

    void processEvent(const SDL_Event *event) const;
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

    ImGui_ImplSDL2_InitForOpenGL(ctx->sdlWindow, ctx->sdlContext);
#if defined(TARGET_OS_OSX)
    ImGui_ImplOpenGL3_Init("#version 330");
#else
    ImGui_ImplOpenGL3_Init("#version 300 es");
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

void ModImgui::processEvent(const SDL_Event *event) const
{
    ImGui_ImplSDL2_ProcessEvent(&(*event));
}
