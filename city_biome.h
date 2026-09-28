#pragma once

#include "framework/gl_header.h"
#include "framework/boot.h"
#include "city.h"
#include "texture.h"

// Level 12's self-contained backdrop. It deliberately does not share the
// neon biome's renderer: its foreground is a field of city blocks, not tiles.
struct CityBiome
{
    GLuint vao = 0;
    GLuint shaderId = 0;
    float time = 0.0f;
    CityBoxMesh groundMesh;
    CityBoxMesh suburbMesh;
    CityBoxMesh roofMesh;
    CityBoxMesh treeMesh;
    CityBoxMesh skylineMesh;
    bool suburbsGenerated = false;

    static constexpr int kSuburbRows = 22;
    static constexpr int kHomesPerSide = 7;
    static constexpr float kSuburbRowSpacing = 12.0f;
    static constexpr float kSuburbStartZ = -40.0f;
    static constexpr float kSuburbBaseY = -17.7f;
    static constexpr float kSuburbScrollSpeed = 1.45f;

    static const char *VERTEX_SHADER;
    static const char *FRAGMENT_SHADER;

    void init()
    {
        if (vao == 0)
        {
            const GLfloat vertices[] = {
                -1.0f, -1.0f,  0.0f, 0.0f,
                 1.0f, -1.0f,  1.0f, 0.0f,
                -1.0f,  1.0f,  0.0f, 1.0f,
                 1.0f,  1.0f,  1.0f, 1.0f,
            };
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
            GLuint vbo = 0;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)(2 * sizeof(GLfloat)));
            glBindVertexArray(0);
        }
        shaderId = vtx::createShaderProgram(VERTEX_SHADER, FRAGMENT_SHADER);
        time = 0.0f;
    }

    void render(float deltaTime)
    {
        time += deltaTime * 0.12f; // deliberate slow city drift
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(shaderId);
        glUniform1f(glGetUniformLocation(shaderId, "uTime"), time);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
    }

    static float hash01(int x, int z)
    {
        return City::hash01(x + 617, z + 1103);
    }

    static void buildPyramidMesh(CityBoxMesh &mesh)
    {
        mesh.mesh.releaseGpu();
        mesh.vertices.clear();
        mesh.indices.clear();
        auto vertex = [&](const glm::vec3 &p, const glm::vec3 &n)
        {
            Vertex v{};
            v.position = {p.x, p.y, p.z};
            v.normal = {n.x, n.y, n.z};
            v.color = {1.0f, 1.0f, 1.0f, 1.0f};
            mesh.vertices.push_back(v);
            return uint32_t(mesh.vertices.size() - 1);
        };
        auto triangle = [&](const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec3 &n)
        {
            mesh.indices.push_back(vertex(a, n));
            mesh.indices.push_back(vertex(b, n));
            mesh.indices.push_back(vertex(c, n));
        };
        const glm::vec3 a(-0.5f, -0.5f, -0.5f), b(0.5f, -0.5f, -0.5f);
        const glm::vec3 c(0.5f, -0.5f, 0.5f), d(-0.5f, -0.5f, 0.5f), top(0.0f, 0.5f, 0.0f);
        triangle(a, b, top, glm::vec3(0.0f, 0.7f, -0.7f));
        triangle(b, c, top, glm::vec3(0.7f, 0.7f, 0.0f));
        triangle(c, d, top, glm::vec3(0.0f, 0.7f, 0.7f));
        triangle(d, a, top, glm::vec3(-0.7f, 0.7f, 0.0f));
        triangle(a, d, c, glm::vec3(0.0f, -1.0f, 0.0f));
        triangle(a, c, b, glm::vec3(0.0f, -1.0f, 0.0f));
        mesh.meshData.vertexCount = uint32_t(mesh.vertices.size());
        mesh.meshData.indexCount = uint32_t(mesh.indices.size());
        mesh.meshData.vertices = mesh.vertices.data();
        mesh.meshData.indices = mesh.indices.data();
        mesh.mesh.sendMeshDataToGpu(&mesh.meshData);
    }

    void buildSuburbs()
    {
        if (suburbsGenerated)
            return;

        City::buildUnitBoxMesh(groundMesh, 1.0f, 1.0f, 1.0f);
        City::buildUnitBoxMesh(suburbMesh, 1.0f, 1.0f, 1.0f);
        City::buildUnitBoxMesh(roofMesh, 1.0f, 1.0f, 1.0f);
        buildPyramidMesh(treeMesh);
        City::buildUnitBoxMesh(skylineMesh, 1.0f, 1.0f, 1.0f);
        // sendMeshDataToGpu creates one identity instance by default.  The
        // ground is custom-instanced below, so discard that origin cube; it
        // otherwise renders as a black box directly over the pin deck.
        groundMesh.mesh.instanceData.clear();
        InstanceData ground{};
        ground.instRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        ground.textureScale = glm::vec3(0.22f, 0.02f, 2.2f);
        ground.positionOffset = glm::vec3(0.0f, kSuburbBaseY - 0.12f, 130.0f);
        ground.scaleOffset = glm::vec3(170.0f, 0.24f, 340.0f);
        groundMesh.mesh.instanceData.push_back(ground);
        groundMesh.mesh.sendInstanceDataToGpu();
        suburbMesh.mesh.instanceData.clear();
        roofMesh.mesh.instanceData.clear();
        treeMesh.mesh.instanceData.clear();
        suburbMesh.mesh.instanceData.reserve(kSuburbRows * kHomesPerSide * 2);
        roofMesh.mesh.instanceData.reserve(kSuburbRows * kHomesPerSide * 4);
        treeMesh.mesh.instanceData.reserve(kSuburbRows * kHomesPerSide * 2);
        for (int row = 0; row < kSuburbRows; ++row)
        {
            for (int side = 0; side < 2; ++side)
            {
                const float sideSign = side == 0 ? -1.0f : 1.0f;
                for (int home = 0; home < kHomesPerSide; ++home)
                {
                    const int seedX = side * 100 + home;
                    const float width = glm::mix(3.4f, 5.7f, hash01(seedX, row));
                    const float depth = glm::mix(4.0f, 6.8f, hash01(seedX + 29, row));
                    const float height = glm::mix(1.9f, 4.75f, hash01(seedX + 57, row));
                    // This is a narrow bowling-lane clearance, not a road:
                    // no building may enter the pin sightline at x == 0.
                    const float lotX = sideSign * (5.5f + float(home) * 10.5f);
                    const float x = lotX + (hash01(seedX + 83, row) - 0.5f) * 2.0f;
                    const float z = kSuburbStartZ + float(row) * kSuburbRowSpacing + (hash01(seedX + 101, row) - 0.5f) * 4.5f;
                    const bool turned = hash01(seedX + 119, row) > 0.57f;
                    const glm::quat yaw = turned
                        ? glm::angleAxis(1.5707963f, glm::vec3(0.0f, 1.0f, 0.0f))
                        : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

                    InstanceData house{};
                    house.instRot = yaw;
                    house.textureScale = glm::vec3(0.08f, 0.22f, 0.08f);
                    house.positionOffset = glm::vec3(x, kSuburbBaseY + height * 0.5f, z);
                    house.scaleOffset = glm::vec3(width, height, depth);
                    house.atlasStart = glm::vec2(0.0f);
                    suburbMesh.mesh.instanceData.push_back(house);

                    // Two sloped panels make a gabled suburban roof instead
                    // of a row of flat-topped boxes.
                    for (int panel = 0; panel < 2; ++panel)
                    {
                        const float panelSign = panel == 0 ? -1.0f : 1.0f;
                        InstanceData roof{};
                        roof.instRot = yaw * glm::angleAxis(-panelSign * 0.52f, glm::vec3(0.0f, 0.0f, 1.0f));
                        roof.textureScale = glm::vec3(0.08f, 0.10f, 0.08f);
                        const glm::vec3 roofOffset = turned
                            ? glm::vec3(0.0f, 0.0f, -panelSign * width * 0.25f)
                            : glm::vec3(panelSign * width * 0.25f, 0.0f, 0.0f);
                        roof.positionOffset = glm::vec3(x, kSuburbBaseY + height + 0.56f, z) + roofOffset;
                        roof.scaleOffset = glm::vec3(width * 0.58f, 0.22f, depth * 1.08f);
                        roof.atlasStart = glm::vec2(0.0f);
                        roofMesh.mesh.instanceData.push_back(roof);
                    }

                    const float treeX = x + sideSign * (width * 0.65f + 1.2f + hash01(seedX + 131, row));
                    InstanceData tree{};
                    tree.instRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                    tree.textureScale = glm::vec3(0.06f, 0.12f, 0.06f);
                    tree.positionOffset = glm::vec3(treeX, kSuburbBaseY + 2.0f, z + (hash01(seedX + 149, row) - 0.5f) * depth);
                    tree.scaleOffset = glm::vec3(2.0f, 4.0f + hash01(seedX + 167, row) * 2.5f, 2.0f);
                    tree.atlasStart = glm::vec2(0.0f);
                    treeMesh.mesh.instanceData.push_back(tree);
                }
            }
        }
        suburbMesh.mesh.sendInstanceDataToGpu();
        roofMesh.mesh.sendInstanceDataToGpu();
        treeMesh.mesh.sendInstanceDataToGpu();

        // A permanent, distant city line. It is genuine world geometry, so it
        // stays planted behind the pins as the player yaws the camera.
        skylineMesh.mesh.instanceData.clear();
        skylineMesh.mesh.instanceData.reserve(46);
        for (int column = 0; column < 46; ++column)
        {
            const float seed = hash01(column + 211, 79);
            const float width = glm::mix(5.5f, 12.0f, seed);
            const float height = glm::mix(14.0f, 42.0f, hash01(column + 239, 101));
            const float x = -160.0f + float(column) * 7.0f;
            InstanceData tower{};
            tower.instRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            tower.textureScale = glm::vec3(0.08f, 0.28f, 0.08f);
            tower.positionOffset = glm::vec3(x, kSuburbBaseY + height * 0.5f, 260.0f + seed * 18.0f);
            tower.scaleOffset = glm::vec3(width, height, glm::mix(5.0f, 12.0f, hash01(column + 271, 131)));
            tower.atlasStart = glm::vec2(0.0f);
            skylineMesh.mesh.instanceData.push_back(tower);
        }
        skylineMesh.mesh.sendInstanceDataToGpu();
        suburbsGenerated = true;
    }

    void updateSuburbs(float deltaTime)
    {
        buildSuburbs();
        const float cycle = kSuburbRows * kSuburbRowSpacing;
        auto advance = [&](std::vector<InstanceData> &instances)
        {
            for (InstanceData &instance : instances)
            {
                instance.positionOffset.z -= deltaTime * kSuburbScrollSpeed;
                while (instance.positionOffset.z < kSuburbStartZ - kSuburbRowSpacing)
                    instance.positionOffset.z += cycle;
            }
        };
        advance(suburbMesh.mesh.instanceData);
        advance(roofMesh.mesh.instanceData);
        advance(treeMesh.mesh.instanceData);
        suburbMesh.mesh.sendInstanceDataToGpu();
        roofMesh.mesh.sendInstanceDataToGpu();
        treeMesh.mesh.sendInstanceDataToGpu();
    }

    void renderSuburbs(ShaderProgram &shader, Texture &texture, const glm::mat4 &view, const glm::mat4 &projection)
    {
        buildSuburbs();
        shader.updateDiffuseTexture(texture);
        shader.updateUseTextureAlpha(false);
        shader.updateAtlasRect(glm::vec3(0.22f, 0.02f, 2.2f), City::kNeonBuildingAtlas.start, City::kNeonBuildingAtlas.size);
        shader.updateColorTintMix(glm::vec3(0.015f, 0.050f, 0.12f), 0.94f, 1.0f);
        shader.renderRealMesh(groundMesh.mesh, glm::mat4(1.0f), view, projection);
        shader.updateAtlasRect(glm::vec3(0.08f, 0.22f, 0.08f), City::kNeonBuildingAtlas.start, City::kNeonBuildingAtlas.size);
        shader.updateColorTintMix(glm::vec3(0.12f, 0.52f, 0.82f), 0.80f, 1.0f);
        shader.renderRealMesh(suburbMesh.mesh, glm::mat4(1.0f), view, projection);
        shader.updateAtlasRect(glm::vec3(0.08f, 0.10f, 0.08f), City::kNeonBuildingAtlas.start, City::kNeonBuildingAtlas.size);
        shader.updateColorTintMix(glm::vec3(0.12f, 0.20f, 0.32f), 0.94f, 1.0f);
        shader.renderRealMesh(roofMesh.mesh, glm::mat4(1.0f), view, projection);
        shader.updateAtlasRect(glm::vec3(0.06f, 0.12f, 0.06f), City::kNeonBuildingAtlas.start, City::kNeonBuildingAtlas.size);
        shader.updateColorTintMix(glm::vec3(0.05f, 0.30f, 0.18f), 0.90f, 1.0f);
        shader.renderRealMesh(treeMesh.mesh, glm::mat4(1.0f), view, projection);
        shader.updateAtlasRect(glm::vec3(0.08f, 0.28f, 0.08f), City::kNeonBuildingAtlas.start, City::kNeonBuildingAtlas.size);
        shader.updateColorTintMix(glm::vec3(0.08f, 0.36f, 0.68f), 0.82f, 1.0f);
        shader.renderRealMesh(skylineMesh.mesh, glm::mat4(1.0f), view, projection);
        shader.updateColorTintMix(glm::vec3(1.0f), 0.0f, 1.0f);
    }
};

const char *CityBiome::VERTEX_SHADER = GLSL_VERSION R"(
precision highp float;
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUv;
out vec2 vUv;
void main() { vUv = aUv; gl_Position = vec4(aPos, 0.0, 1.0); }
)";

const char *CityBiome::FRAGMENT_SHADER = GLSL_VERSION R"(
precision highp float;
in vec2 vUv;
out vec4 FragColor;
uniform float uTime;

float hash(float n) { return fract(sin(n) * 43758.5453123); }

// A building facade with a sparse, animated window pattern.
vec3 facade(vec2 uv, float floorY, float height, float seed) {
    float inside = step(floorY, uv.y) * step(uv.y, floorY + height);
    float windowX = step(0.70, fract(uv.x * 38.0 + seed));
    float windowY = step(0.64, fract((uv.y - floorY) * 76.0 + seed * 2.0));
    float lit = windowX * windowY * step(0.35, hash(floor(seed * 13.0 + floor(uv.y * 76.0))));
    vec3 wall = mix(vec3(0.010, 0.022, 0.050), vec3(0.025, 0.080, 0.145), fract(seed * 9.0));
    return mix(vec3(0.30, 0.72, 1.0), wall, 1.0 - lit) * inside;
}

void main() {
    vec2 uv = vUv;

    // Keep the aurora visible behind this transparent skyline.  The raised
    // profile lets the buildings break clearly above the lane horizon.
    float cell = floor(uv.x * 23.0);
    float seed = hash(cell + 17.0);
    float skyline = 0.58 + seed * 0.18 + sin(cell * 1.7) * 0.025;
    float buildingMask = step(uv.y, skyline);
    vec3 color = facade(uv, 0.0, skyline, seed);
    color *= 1.0 - 0.24 * dot(uv - 0.5, uv - 0.5);
    FragColor = vec4(color, buildingMask * 0.94);
}
)";
