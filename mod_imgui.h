#pragma once

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "framework/ctx.h"

struct MyImGui
{
    ImGuiContext* imGuiCtx;

    ImFont *font = nullptr;
    ImFont *custom_font = nullptr;

    void initImgui(vtx::VertexContext *ctx);
    void loadImgui(vtx::VertexContext *ctx);
    void hangImgui(vtx::VertexContext *ctx);

    void processEvent(const SDL_Event *event) const;
    void newFrame() const;
    void renderFrame() const;
    void showMatrixEditor(glm::mat4 *matrix, const char *title) const;
    void loadImgui();
};

void MyImGui::hangImgui(vtx::VertexContext *ctx)
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void MyImGui::loadImgui(vtx::VertexContext *ctx)
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

void MyImGui::initImgui(vtx::VertexContext *ctx)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(ctx->sdlWindow, ctx->sdlContext);

    this->imGuiCtx = ImGui::GetCurrentContext();
    std::cerr << "Store imgui at " << this->imGuiCtx << std::endl;
}

void MyImGui::processEvent(const SDL_Event *event) const
{
    ImGui_ImplSDL2_ProcessEvent(&(*event));
}

void MyImGui::newFrame() const
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void MyImGui::renderFrame() const
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void MyImGui::showMatrixEditor(glm::mat4 *matrix, const char *title)
    const
{
    if (ImGui::Begin(
            title, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::BeginTable("Matrix", 4, ImGuiTableFlags_Borders))
        {
            for (int row = 0; row < 4; row++)
            {
                ImGui::TableNextRow();
                for (int col = 0; col < 4; col++)
                {
                    ImGui::TableNextColumn();

                    // Use InputFloat to edit each element in the
                    // matrix
                    char label[32];
                    snprintf(label, sizeof(label), "##%d%d", row, col);

                    // Access the matrix element
                    ImGui::SetNextItemWidth(90); // Set the input width
                    ImGui::InputFloat(
                        label, &(*matrix)[col][row], 0.01f, 1.0f, "%.2f");
                    // Note: glm::mat4 is column-major, so we use
                }
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}
