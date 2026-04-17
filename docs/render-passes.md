
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