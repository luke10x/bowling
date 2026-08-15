
## Pass Breakdown

### Pass 1: 3D World (Perspective)
- **Projection**: `glm::perspective(...)`
- **Depth State**: `glEnable(GL_DEPTH_TEST)`, `glDepthFunc(GL_LESS)`, `glDepthMask(GL_TRUE)`
- **Purpose**: Renders the main scene. Populates the depth buffer with perspective-correct Z values.
- **Culling**: Enabled. Standard backface removal.

### Pass 2: Clay UI (2D Overlay)
- **Projection**: `glm::ortho(0, W, H, 0, -1, 1)` (Y-down screen coordinates)
- **Depth State**: `glDisable(GL_DEPTH_TEST)`
- **Purpose**: Renders 2D interface elements. Ignores scene depth to guarantee UI draws on top.
- **Note**: UI elements typically don't write to depth, so clearing depth later is safe.

### Pass 3: Flying Coins (3D Overlay)
- **Projection**: `glm::ortho(0, W, H, 0, -1, 1)` (matches UI screen space)
- **Depth State**: `glClear(GL_DEPTH_BUFFER_BIT)` → `glEnable(GL_DEPTH_TEST)`, `glDepthMask(GL_TRUE)`
- **Purpose**: Renders sophisticated 3D coin geometry that must **self-occlude** while visually sitting on top of UI and scene.

## Why Clear the Depth Buffer?
Orthographic projection maps Z linearly to `[-1, 1]`. The 3D world's depth buffer contains **perspective** Z values that don't align with ortho Z. If you enable depth testing without clearing:
- Coins fail `GL_LESS` against scene fragments
- Entire coin mesh gets clipped

Clearing `GL_DEPTH_BUFFER_BIT` gives coins a **fresh depth buffer** where they sort against themselves correctly, while still appearing visually on top of everything else.

## Preview Texture Timing Trap

Clayton only exposes four image texture slots:
- Slot 0: the everything texture.
- Slots 1-3: framebuffer-backed preview textures.

Those slots are bindings, not ownership. If two UI elements need the same preview image, prefer rendering them in the same early preview-texture phase rather than re-rendering one later during the Clay/UI phase.

This matters because late preview renders inherit state from the main 3D world pass. The world pass may have just drawn balls, ball shells, traces, chests, particles, decals, or changed depth/blend/scissor/color-mask state. A late render into `ballRenderTex` can look like a foreign round object is in front of the camera, even when the camera and mesh are correct.

Rule of thumb: render FBO previews before the main 3D world pass whenever possible, then let Clay only sample the finished texture. If a preview must render late, fully reset GL state first: disable scissor/cull/blend, restore full color writes, set `glDepthFunc(GL_LESS)`, set `glDepthMask(GL_TRUE)`, and clear color + depth before drawing.

## State Transition Table

| Pass | Depth Test | Depth Mask | Cull Face | Blend | Depth Buffer Action |
|------|------------|------------|-----------|-------|---------------------|
| 1. World | ✅ `GL_LESS` | ✅ `TRUE` | ✅ `GL_BACK` | ❌ | Populate |
| 2. UI | ❌ `OFF` | ❌ `FALSE` | ❌ `OFF` | ✅ `SRC_ALPHA` | Ignore |
| 3. Coins | ✅ `GL_LEQUAL` | ✅ `TRUE` | ✅ `GL_BACK` | ✅ `SRC_ALPHA` | **`glClear`** → Populate |

## Alternative Approaches (Why Not Used?)

| Approach | Pros | Cons |
|----------|------|------|
| `glDepthFunc(GL_ALWAYS)` | No depth clear needed | Breaks internal face sorting (backfaces overlap front) |
| Separate FBO for coins | Complete isolation | Extra render target, blit overhead, memory cost |
| Sort coins by Z back-to-front | Works with depth off | Fails for rotating/complex geometry, CPU bound |

**Our chosen approach** (`glClear` + standard depth test) is the industry standard for screen-space 3D overlays. It's GPU-efficient, guarantees correct self-occlusion, and requires zero extra render targets.

## Coordinate System Notes
- **Screen-Space Ortho**: `glm::ortho(0, width, height, 0, near, far)` assumes Y-down (top-left origin), matching most UI frameworks.
- **Model Matrices**: Coins are positioned in **screen pixels**, not world units.
  ```cpp
  glm::mat4 model = glm::translate(..., glm::vec3(screenX, screenY, 0.0f));
  model = glm::scale(..., glm::vec3(pixelSize));

- Z Values: Keep model Z within [-0.9, 0.9] to stay safely inside ortho clip space.

## Best Practices

1. Always restore OpenGL state after Pass 3 if rendering continues
2. Call glViewport(0, 0, W, H) before Pass 3 (Clay UI may resize viewport)
3. Batch coin draws under the same shader/texture binding to minimize state changes
4. Validate light position is in view space (for screen-space: view = identity, so world=view)
5. Pipeline Flow (Pseudocode)

```cpp
// ===== [NEW] PRE-PASS: Render ball to texture for UI =====
// (Do this AFTER ballModel is computed, BEFORE any rendering)
{
    // Bind FBO + set viewport to match texture size
    usr->ballRenderTex.bindForWriting(); // sets glViewport(0,0,256,256) internally
    
    // Clear with transparency for UI overlay friendliness
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Enable depth for proper ball self-occlusion
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    
    // Render ball using SAME model matrix as main pass (critical for visual match)
    // Use your existing shader — no need for a special one unless you want unlit
    usr->mainShader.renderRealMesh(
        usr->ballMesh, 
        ballModel,              // ← Same matrix used in main world pass
        usr->cameraMat,         // ← Main camera (or use a dedicated "icon cam" if preferred)
        usr->perspectiveMat
    );
    
    // Return to default framebuffer + restore main viewport
    usr->ballRenderTex.unbind(); // restores viewport to screenWidth/Height
}
// ===== [END NEW PRE-PASS] =====
  // Pass 1: World
glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE);
renderWorld(perspectiveProj, cameraView);

// Pass 2: UI
glDisable(GL_DEPTH_TEST);
renderClayUI(orthoUIProj);

// Pass 3: Coins Overlay
glClear(GL_DEPTH_BUFFER_BIT);
glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE);
glEnable(GL_CULL_FACE); glCullFace(GL_BACK);
glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

simpleShader.updateDiffuseTexture(coinTex);
simpleShader.updateLightParams(lightPos, lightColor, ambient);
for (auto &fly : animations) {
    simpleShader.renderSimpleMesh(coinMesh, fly.model, identityView, orthoProj);
}

// Restore
glDisable(GL_BLEND);
glDisable(GL_CULL_FACE);
glDepthMask(GL_TRUE);
glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS);
