#pragma once

#include "framework/gl_header.h"
#include "framework/boot.h"
#include "city.h"
#include "texture.h"

// A soft particle quad, deliberately separate from the box meshes used for
// buildings and wrecks.  Smoke must never inherit the hard silhouette of a
// building primitive.
struct RuinSmokeVertex
{
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec4 params;
};

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
    CityBoxMesh ruinGroundMesh;
    CityBoxMesh ruinBuildingMesh;
    CityBoxMesh wreckMesh;
    CityBoxMesh fireMesh;
    struct RuinParticle { glm::vec3 base; glm::vec3 scale; float phase; };
    std::vector<RuinParticle> fireParticles;
    std::vector<RuinParticle> smokeParticles;
    GLuint ruinSmokeVao = 0;
    GLuint ruinSmokeVbo = 0;
    GLuint ruinSmokeEbo = 0;
    GLuint ruinSmokeShaderId = 0;
    GLsizei ruinSmokeIndexCount = 0;
    std::vector<RuinSmokeVertex> ruinSmokeVertices;
    std::vector<uint32_t> ruinSmokeIndices;
    float ruinSmokeScrollZ = 0.0f;
    bool suburbsGenerated = false;
    bool ruinsGenerated = false;

    static constexpr int kSuburbRows = 22;
    static constexpr int kHomesPerSide = 7;
    static constexpr float kSuburbRowSpacing = 12.0f;
    static constexpr float kSuburbStartZ = -40.0f;
    static constexpr float kSuburbBaseY = -17.7f;
    static constexpr float kSuburbScrollSpeed = 1.45f;

    static const char *VERTEX_SHADER;
    static const char *FRAGMENT_SHADER;
    static const char *RUIN_SMOKE_VERTEX_SHADER;
    static const char *RUIN_SMOKE_FRAGMENT_SHADER;

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

    void buildRuinSmokeMesh()
    {
        ruinSmokeVertices.clear();
        ruinSmokeIndices.clear();
        ruinSmokeVertices.reserve(smokeParticles.size() * 4);
        ruinSmokeIndices.reserve(smokeParticles.size() * 6);
        auto pushQuad = [&](const RuinParticle &p, int index)
        {
            const float width = p.scale.x * (2.8f + hash01(index, 811) * 1.4f);
            const float height = p.scale.y * (3.0f + hash01(index, 823) * 1.5f);
            // The bottom edge begins at the fire source, so the cloud reads
            // as a plume rising from a flame rather than a floating decal.
            const float lift = height * 0.5f;
            const glm::vec3 center = p.base + glm::vec3(
                (hash01(index, 829) - 0.5f) * width * 0.35f,
                lift,
                (hash01(index, 839) - 0.5f) * width * 0.12f
            );
            const uint32_t base = uint32_t(ruinSmokeVertices.size());
            const glm::vec3 right(width * 0.5f, 0.0f, 0.0f);
            const glm::vec3 up(0.0f, height * 0.5f, 0.0f);
            const glm::vec4 params(p.phase, width, height, 0.56f + hash01(index, 853) * 0.28f);
            ruinSmokeVertices.push_back({center - right - up, glm::vec2(0.0f, 0.0f), params});
            ruinSmokeVertices.push_back({center + right - up, glm::vec2(1.0f, 0.0f), params});
            ruinSmokeVertices.push_back({center + right + up, glm::vec2(1.0f, 1.0f), params});
            ruinSmokeVertices.push_back({center - right + up, glm::vec2(0.0f, 1.0f), params});
            ruinSmokeIndices.insert(ruinSmokeIndices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
        };
        for (size_t i = 0; i < smokeParticles.size(); ++i)
            pushQuad(smokeParticles[i], int(i));

        ruinSmokeIndexCount = GLsizei(ruinSmokeIndices.size());
        if (ruinSmokeShaderId == 0)
            ruinSmokeShaderId = vtx::createShaderProgram(RUIN_SMOKE_VERTEX_SHADER, RUIN_SMOKE_FRAGMENT_SHADER);
        if (ruinSmokeEbo != 0) glDeleteBuffers(1, &ruinSmokeEbo);
        if (ruinSmokeVbo != 0) glDeleteBuffers(1, &ruinSmokeVbo);
        if (ruinSmokeVao != 0) glDeleteVertexArrays(1, &ruinSmokeVao);
        glGenVertexArrays(1, &ruinSmokeVao);
        glGenBuffers(1, &ruinSmokeVbo);
        glGenBuffers(1, &ruinSmokeEbo);
        glBindVertexArray(ruinSmokeVao);
        glBindBuffer(GL_ARRAY_BUFFER, ruinSmokeVbo);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(ruinSmokeVertices.size() * sizeof(RuinSmokeVertex)), ruinSmokeVertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ruinSmokeEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(ruinSmokeIndices.size() * sizeof(uint32_t)), ruinSmokeIndices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RuinSmokeVertex), (void *)offsetof(RuinSmokeVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(RuinSmokeVertex), (void *)offsetof(RuinSmokeVertex, uv));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(RuinSmokeVertex), (void *)offsetof(RuinSmokeVertex, params));
        glBindVertexArray(0);
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

    void buildRuins()
    {
        if (ruinsGenerated)
            return;
        City::buildUnitBoxMesh(ruinGroundMesh, 1.0f, 1.0f, 1.0f);
        City::buildUnitBoxMesh(ruinBuildingMesh, 1.0f, 1.0f, 1.0f);
        City::buildUnitBoxMesh(wreckMesh, 1.0f, 1.0f, 1.0f);
        buildPyramidMesh(fireMesh);

        ruinGroundMesh.mesh.instanceData.clear();
        InstanceData ground{};
        ground.instRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        ground.textureScale = glm::vec3(0.24f, 0.02f, 2.5f);
        ground.positionOffset = glm::vec3(0.0f, kSuburbBaseY - 0.15f, 135.0f);
        ground.scaleOffset = glm::vec3(190.0f, 0.30f, 350.0f);
        ruinGroundMesh.mesh.instanceData.push_back(ground);
        ruinGroundMesh.mesh.sendInstanceDataToGpu();

        ruinBuildingMesh.mesh.instanceData.clear();
        wreckMesh.mesh.instanceData.clear();
        fireMesh.mesh.instanceData.clear();
        fireParticles.clear();
        smokeParticles.clear();
        for (int row = 0; row < 24; ++row)
        {
            for (int side = 0; side < 2; ++side)
            {
                const float sideSign = side == 0 ? -1.0f : 1.0f;
                for (int lot = 0; lot < 5; ++lot)
                {
                    const int seed = side * 200 + row * 11 + lot;
                    const float width = glm::mix(5.0f, 12.0f, hash01(seed, 7));
                    const float height = glm::mix(7.0f, 30.0f, hash01(seed, 19));
                    const float depth = glm::mix(6.0f, 14.0f, hash01(seed, 83));
                    const float x = sideSign * (7.0f + lot * 11.5f + hash01(seed, 31) * 2.5f);
                    const float z = -35.0f + row * 13.5f + (hash01(seed, 43) - 0.5f) * 4.0f;
                    const bool collapsed = hash01(seed, 59) > 0.58f;
                    InstanceData building{};
                    building.instRot = collapsed
                        ? glm::angleAxis(sideSign * glm::mix(0.24f, 0.58f, hash01(seed, 71)), glm::vec3(0.0f, 0.0f, 1.0f))
                        : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                    building.textureScale = glm::vec3(0.08f, 0.28f, 0.08f);
                    building.positionOffset = glm::vec3(x, kSuburbBaseY + (collapsed ? height * 0.18f : height * 0.5f), z);
                    building.scaleOffset = glm::vec3(width, collapsed ? height * 0.36f : height, depth);
                    building.atlasStart = glm::vec2(0.0f);
                    ruinBuildingMesh.mesh.instanceData.push_back(building);

                    // Fires still favor the lower, collapsed remains, but the
                    // denser destruction is spread across many buildings.
                    const bool burningBuilding = collapsed
                        ? hash01(seed, 97) > 0.04f
                        : hash01(seed, 97) > 0.25f;
                    if (burningBuilding)
                    {
                        // One distinct burn site per selected building.  The
                        // increased density comes from more buildings burning,
                        // not duplicated sites on the same facade.
                        for (int site = 0; site < 1; ++site)
                        {
                            const int siteSeed = seed + site * 503;
                            const glm::vec3 source(
                                x + (hash01(siteSeed, 101) - 0.5f) * width * 0.45f,
                                kSuburbBaseY + (collapsed ? height * 0.42f : height),
                                z - depth * 0.5f - 0.20f
                            );
                            for (int particle = 0; particle < 10; ++particle)
                            {
                                const float phase = hash01(siteSeed + particle * 23, 109) * 6.2831853f;
                                RuinParticle flame = {
                                    source + glm::vec3((hash01(siteSeed + particle, 127) - 0.5f) * 2.0f, 0.0f, (hash01(siteSeed + particle, 139) - 0.5f) * 2.0f),
                                    glm::vec3(0.55f + hash01(siteSeed + particle, 149) * 0.75f, 1.4f + hash01(siteSeed + particle, 151) * 2.8f, 0.55f + hash01(siteSeed + particle, 157) * 0.75f),
                                    phase
                                };
                                fireParticles.push_back(flame);
                                InstanceData fire{};
                                fire.instRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                                fire.textureScale = glm::vec3(0.08f);
                                fire.positionOffset = flame.base;
                                fire.scaleOffset = flame.scale;
                                fire.atlasStart = glm::vec2(0.0f);
                                fireMesh.mesh.instanceData.push_back(fire);
                            }
                            for (int particle = 0; particle < 5; ++particle)
                            {
                                RuinParticle smoke = {source, glm::vec3(1.6f, 2.4f, 1.6f), hash01(siteSeed + particle, 173) * 6.2831853f};
                                smokeParticles.push_back(smoke);
                            }
                        }
                    }
                }
            }
        }
        for (int wreck = 0; wreck < 42; ++wreck)
        {
            const float seed = hash01(wreck, 331);
            InstanceData car{};
            car.instRot = glm::angleAxis(hash01(wreck, 347) * 6.2831853f, glm::vec3(0.0f, 1.0f, 0.0f));
            car.textureScale = glm::vec3(0.16f);
            car.positionOffset = glm::vec3(glm::mix(-24.0f, 24.0f, seed), kSuburbBaseY + 0.48f, -20.0f + wreck * 7.0f);
            car.scaleOffset = glm::vec3(glm::mix(0.8f, 1.4f, hash01(wreck, 359)), 0.65f, glm::mix(1.8f, 3.8f, hash01(wreck, 367)));
            car.atlasStart = glm::vec2(0.0f);
            wreckMesh.mesh.instanceData.push_back(car);
            if (hash01(wreck, 379) > 0.0f)
            {
                // A few accidents are burning on the roadway beside their
                // wreck, rather than as flames floating inside the vehicle.
                const glm::vec3 source = car.positionOffset + glm::vec3(
                    0.0f,
                    -0.20f,
                    -car.scaleOffset.z * 0.62f
                );
                for (int particle = 0; particle < 7; ++particle)
                {
                    RuinParticle flame = {source, glm::vec3(0.55f, 1.8f, 0.55f), hash01(wreck + particle, 389) * 6.2831853f};
                    fireParticles.push_back(flame);
                    InstanceData instance{};
                    instance.instRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                    instance.textureScale = glm::vec3(0.08f);
                    instance.positionOffset = flame.base;
                    instance.scaleOffset = flame.scale;
                    fireMesh.mesh.instanceData.push_back(instance);
                }
                RuinParticle smoke = {source, glm::vec3(2.4f, 1.4f, 1.0f), hash01(wreck, 397) * 6.2831853f};
                smokeParticles.push_back(smoke);
            }
        }
        ruinBuildingMesh.mesh.sendInstanceDataToGpu();
        wreckMesh.mesh.sendInstanceDataToGpu();
        fireMesh.mesh.sendInstanceDataToGpu();
        buildRuinSmokeMesh();
        ruinsGenerated = true;
    }

    void updateRuins(float deltaTime)
    {
        const float speed = City::kCityScrollSpeed;
        const float cycle = 24.0f * 13.5f;
        const float nearLimit = kSuburbStartZ - 13.5f;
        auto advanceInstances = [&](std::vector<InstanceData> &instances)
        {
            for (InstanceData &instance : instances)
            {
                instance.positionOffset.z -= deltaTime * speed;
                while (instance.positionOffset.z < nearLimit)
                    instance.positionOffset.z += cycle;
            }
        };
        advanceInstances(ruinBuildingMesh.mesh.instanceData);
        advanceInstances(wreckMesh.mesh.instanceData);
        for (RuinParticle &particle : fireParticles)
        {
            particle.base.z -= deltaTime * speed;
            while (particle.base.z < nearLimit)
                particle.base.z += cycle;
        }
        ruinBuildingMesh.mesh.sendInstanceDataToGpu();
        wreckMesh.mesh.sendInstanceDataToGpu();
        ruinSmokeScrollZ = glm::mod(ruinSmokeScrollZ + deltaTime * speed, cycle);
    }

    void renderRuins(float deltaTime, ShaderProgram &shader, Texture &texture, const glm::mat4 &view, const glm::mat4 &projection)
    {
        buildRuins();
        time += deltaTime;
        updateRuins(deltaTime);
        const float t = time;
        for (size_t i = 0; i < fireParticles.size(); ++i)
        {
            const RuinParticle &particle = fireParticles[i];
            const float rise = 0.45f + 0.5f * sin(t * 5.0f + particle.phase);
            fireMesh.mesh.instanceData[i].positionOffset = particle.base + glm::vec3(sin(t * 3.0f + particle.phase) * 0.22f, rise, cos(t * 2.0f + particle.phase) * 0.22f);
            fireMesh.mesh.instanceData[i].scaleOffset = particle.scale * (0.78f + 0.26f * sin(t * 6.0f + particle.phase));
        }
        fireMesh.mesh.sendInstanceDataToGpu();
        shader.updateDiffuseTexture(texture);
        shader.updateUseTextureAlpha(false);
        shader.updateAtlasRect(glm::vec3(0.24f, 0.02f, 2.5f), City::kNeonBuildingAtlas.start, City::kNeonBuildingAtlas.size);
        shader.updateColorTintMix(glm::vec3(0.08f, 0.06f, 0.07f), 0.96f, 1.0f);
        shader.renderRealMesh(ruinGroundMesh.mesh, glm::mat4(1.0f), view, projection);
        shader.updateAtlasRect(glm::vec3(0.08f, 0.28f, 0.08f), City::kNeonBuildingAtlas.start, City::kNeonBuildingAtlas.size);
        shader.updateColorTintMix(glm::vec3(0.22f, 0.13f, 0.12f), 0.88f, 1.0f);
        shader.renderRealMesh(ruinBuildingMesh.mesh, glm::mat4(1.0f), view, projection);
        shader.updateAtlasRect(glm::vec3(0.16f), City::kNeonBuildingAtlas.start, City::kNeonBuildingAtlas.size);
        shader.updateColorTintMix(glm::vec3(0.24f, 0.07f, 0.03f), 0.92f, 1.0f);
        shader.renderRealMesh(wreckMesh.mesh, glm::mat4(1.0f), view, projection);
        shader.updateAtlasRect(glm::vec3(0.08f), City::kNeonBuildingAtlas.start, City::kNeonBuildingAtlas.size);
        // Soft particle smoke in the same world/depth space as the fire.
        // It remains translucent, but buildings correctly occlude it.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glUseProgram(ruinSmokeShaderId);
        glUniformMatrix4fv(glGetUniformLocation(ruinSmokeShaderId, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(ruinSmokeShaderId, "u_projection"), 1, GL_FALSE, glm::value_ptr(projection));
        const glm::vec3 cameraPos = glm::vec3(glm::inverse(view)[3]);
        glUniform3fv(glGetUniformLocation(ruinSmokeShaderId, "u_cameraPos"), 1, glm::value_ptr(cameraPos));
        glUniform1f(glGetUniformLocation(ruinSmokeShaderId, "u_time"), t);
        glUniform1f(glGetUniformLocation(ruinSmokeShaderId, "u_scrollZ"), ruinSmokeScrollZ);
        glBindVertexArray(ruinSmokeVao);
        glDrawElements(GL_TRIANGLES, ruinSmokeIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        // Flames deliberately render last with additive blending so the soft
        // smoke can surround them without ever hiding their orange core.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        shader.updateColorTintMix(glm::vec3(1.0f, 0.20f, 0.015f), 0.94f, 0.72f);
        shader.renderRealMesh(fireMesh.mesh, glm::mat4(1.0f), view, projection);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
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

const char *CityBiome::RUIN_SMOKE_VERTEX_SHADER = GLSL_VERSION R"(
precision highp float;
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_params;
uniform mat4 u_worldToView;
uniform mat4 u_projection;
uniform float u_time;
uniform float u_scrollZ;
out vec2 v_uv;
out vec2 v_flowUv;
out vec3 v_worldPos;
out float v_alpha;
out float v_noiseSeed;
out float v_rise01;
void main()
{
    float phase = a_params.x;
    float width = a_params.y;
    float height = a_params.z;
    float cycle = fract(u_time * 0.045 + phase);
    float rise = cycle * 4.2;
    float fadeIn = smoothstep(0.0, 0.10, cycle);
    float fadeOut = 1.0 - smoothstep(0.72, 1.0, cycle);
    vec2 windDir = normalize(vec2(1.0, -0.34));
    vec2 planeDrift = windDir * (cycle * 5.5);
    vec3 pos = a_pos + vec3(
        planeDrift.x + sin(u_time * 0.38 + phase) * (0.42 + width * 0.055),
        rise,
        planeDrift.y + cos(u_time * 0.27 + phase * 1.7) * (0.32 + width * 0.030)
    );
    pos.z -= u_scrollZ;
    // The upper part of a plume opens into a broader cloud as it clears a
    // roofline, instead of retaining the narrow dark source shape.
    float expansion = smoothstep(0.20, 0.95, cycle);
    pos.x += (a_uv.x - 0.5) * width * expansion * 0.95;
    pos.y += (a_uv.y - 0.5) * height * expansion * 0.42;
    float swing = sin(u_time * 0.58 + phase + a_uv.y * 2.4);
    pos.x += (a_uv.y - 0.5) * swing * (1.4 + width * 0.10);
    pos.z += (a_uv.y - 0.5) * cos(u_time * 0.43 + phase) * (0.6 + width * 0.045);
    pos.y += (a_uv.x - 0.5) * cos(u_time * 0.18 + phase) * 0.42;
    v_uv = a_uv;
    v_flowUv = (a_pos.xz + windDir * u_time * 18.0 + vec2(phase * 4.1, -phase * 2.7)) * 0.055;
    v_alpha = fadeIn * fadeOut * mix(0.48, 0.78, a_params.w);
    v_noiseSeed = phase + height;
    v_rise01 = cycle;
    v_worldPos = pos;
    gl_Position = u_projection * u_worldToView * vec4(pos, 1.0);
}
)";

const char *CityBiome::RUIN_SMOKE_FRAGMENT_SHADER = GLSL_VERSION R"(
precision highp float;
in vec2 v_uv;
in vec2 v_flowUv;
in vec3 v_worldPos;
in float v_alpha;
in float v_noiseSeed;
in float v_rise01;
uniform vec3 u_cameraPos;
out vec4 FragColor;
float hash21(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}
float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash21(i), hash21(i + vec2(1.0, 0.0)), f.x),
               mix(hash21(i + vec2(0.0, 1.0)), hash21(i + vec2(1.0, 1.0)), f.x), f.y);
}
void main()
{
    vec2 centered = v_uv * 2.0 - 1.0;
    centered.x *= 0.78;
    centered.y *= 1.12;
    float d = length(centered);
    float softBody = 1.0 - smoothstep(0.08, 1.18, d);
    float feather = 1.0 - smoothstep(0.46, 1.18, d);
    float mottled = noise(v_uv * 2.0 + v_flowUv + vec2(v_noiseSeed, 0.0));
    float wisps = noise(v_uv * 5.8 + v_flowUv * 1.7 + vec2(v_noiseSeed * 0.41, 3.7));
    float cloud = softBody * feather * mix(0.42, 0.92, mottled) * mix(0.70, 1.0, wisps);
    float centerBoost = mix(1.0, 2.0, 1.0 - smoothstep(0.0, 0.58, d));
    float cameraDist = length(v_worldPos.xz - u_cameraPos.xz);
    // Dense at the flame, then naturally thinning as the plume rises.
    float sourceDensity = mix(2.0, 1.0, smoothstep(0.0, 0.72, v_uv.y));
    // Keep only the immediately-under-camera sources quiet.  Nearby wrecks
    // and low fires still need visible smoke as the player passes them.
    float highPlume = smoothstep(0.42, 0.95, v_rise01);
    float alpha = cloud * v_alpha * 0.72 * centerBoost * sourceDensity
        * mix(1.0, 0.46, highPlume) * smoothstep(6.0, 18.0, cameraDist);
    if (alpha < 0.006) discard;
    vec3 smoke = mix(vec3(0.20, 0.16, 0.15), vec3(0.48, 0.42, 0.37), smoothstep(0.0, 1.0, v_uv.y));
    smoke = mix(smoke, vec3(0.82, 0.80, 0.76), highPlume * 0.82);
    FragColor = vec4(smoke, alpha);
}
)";
