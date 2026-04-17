```markdown
# C++ Code Style & Preferences

## Overview
Modern C++17 game development code with focus on performance, hot-reload capability, and timestep independence.

---

## Language Standard
- **C++17** minimum
- Compiler flags: `-std=c++17` or `/std:c++17`

---

## File Organization

### Header-Only Design
```cpp
#pragma once  // Include guard (preferred over #ifndef)

// All implementation in headers
// No separate .cpp files for game logic modules
```

**Rationale**: Faster compilation, easier hot-reload, simpler deployment.

---

## Naming Conventions

### Types (Structs/Enums/Classes)
**PascalCase**
```cpp
struct CoinLane { };
enum class CoinState { Active, Collected, Dead };
struct CoinFlyConfig { };
```

### Variables & Members
**camelCase**
```cpp
int activeCount = 0;
float rotationSpeed = 2.5f;
glm::vec3 position;
```

### Functions & Methods
**camelCase**
```cpp
void updateStars(float deltaTime);
bool hasActiveCoins() const;
void applyPattern(Coin& coin, float globalTime);
```

### Constants
**SCREAMING_SNAKE_CASE** (for static constants)
```cpp
static inline const int MAX_COINS = 10;
static inline const float FLY_DURATION = 0.8f;
static constexpr float PI = 3.14159265358979323846f;
```

### Parameters
**camelCase** (same as variables)
```cpp
void start(const glm::vec2& screenPos, const glm::vec2& target);
```

---

## Modern C++ Features

### `static inline const` for Hot-Reload
```cpp
// ✅ Preferred for tunable game constants
static inline const float FLY_DURATION = 0.8f;

// ❌ Avoid constexpr for values that might change at runtime
// static constexpr float FLY_DURATION = 0.8f;  // Too rigid
```

**Rationale**: `static inline const` allows hot-reloading DLLs/shared libraries to update values without recompiling dependent code.

### `[[nodiscard]]` Attribute
```cpp
[[nodiscard]] bool hasActiveCoins() const noexcept;
[[nodiscard]] int getRenderableCount() const noexcept;
```

**Use when**: 
- Return value is the primary purpose of the function
- Ignoring the result is likely a bug
- Query/getter methods

**Avoid when**:
- Function is called for side effects only
- Return value is optional/auxiliary

### `noexcept` Specification
```cpp
void updateTransform() noexcept;
int getActiveCount() const noexcept;
```

**Use on**:
- All methods that don't throw (which should be most game logic)
- Enables compiler optimizations
- Documents intent clearly

### `const` Correctness
```cpp
// ✅ Const method (doesn't modify object)
[[nodiscard]] int getActiveCount() const noexcept;

// ✅ Const reference parameter (avoids copy, prevents modification)
void start(const glm::vec2& screenPos, const glm::vec2& target);

// ✅ Const iterator
for (const auto& anim : flyAnimations) { }
```

---

## Data Structures

### Prefer `std::array` for Fixed-Size Collections
```cpp
// ✅ Fixed size known at compile time
std::array<Coin, MAX_COINS> coins{};
std::array<CoinFlyAnimation, MAX_FLY_ANIMATIONS> flyAnimations{};

// ❌ Avoid std::vector for fixed-size game objects
// std::vector<Coin> coins;  // Unnecessary heap allocation
```

**Rationale**: 
- Stack allocation (no heap fragmentation)
- Cache-friendly contiguous memory
- Zero initialization overhead
- Bounds checking in debug with `.at()`

### Structs Over Classes
```cpp
// ✅ Public by default, simpler syntax
struct Coin {
    glm::vec3 position;
    float scale;
    // ... all public unless specified
};

// ❌ Avoid unless you need encapsulation
class Coin { 
private:
    // ... only if you truly need private members
};
```

**Rationale**: Game objects are data carriers; encapsulation adds complexity without benefit in performance-critical code.

### Scoped Enums (`enum class`)
```cpp
enum class CoinState : uint8_t {
    Active,
    Collected,
    Dead
};

enum class CoinPattern : uint8_t {
    Static,
    SideToSide,
    WaveBop
};
```

**Benefits**:
- Type-safe (no implicit conversion to int)
- Scoped names (`CoinState::Active` not just `Active`)
- Explicit underlying type (`uint8_t` for memory efficiency)

---

## Memory Management

### Stack Allocation Preferred
```cpp
// ✅ Stack-allocated, automatic lifetime
CoinLane coinLane;
std::array<Coin, 10> coins;

// ❌ Avoid raw pointers and new/delete
// Coin* coin = new Coin();  // Manual memory management
// delete coin;
```

### No Smart Pointers in Hot Path
```cpp
// ❌ Avoid in performance-critical game loops
// std::unique_ptr<Coin> coin = std::make_unique<Coin>();
// std::shared_ptr<Coin> coin = std::make_shared<Coin>();

// ✅ Prefer value semantics or stack allocation
Coin coin;
```

**Exception**: Smart pointers OK for resource management (textures, meshes) outside game loop.

---

## Performance Patterns

### Timestep Independence
```cpp
// ✅ Accumulate real time, not frame count
elapsed += deltaTime;
float t = elapsed / duration;  // Normalized [0,1]

// ❌ Frame-dependent logic
// frameCount++;  // Breaks at different FPS
// if (frameCount % 10 == 0) { }  // Timing varies with FPS
```

### Cap DeltaTime (Safety Only)
```cpp
// ✅ Cap to prevent spiral-of-death, but allow normal progression
deltaTime = std::min(deltaTime, 0.05f);  // ~20 FPS safety cap

// ❌ Don't cap so low it breaks low-FPS gameplay
// deltaTime = std::min(deltaTime, 0.0133f);  // Breaks at <75 FPS
```

### Pass by Const Reference
```cpp
// ✅ Avoids copy for large objects
void updateStars(const glm::vec3& ballPos, float deltaTime);

// ✅ OK for small trivially-copyable types
void update(float deltaTime);  // float is cheap to copy
```

### Inline Small Functions
```cpp
// ✅ Implicit inline in header
void updateTransform() noexcept {
    transform = glm::translate(glm::mat4(1.0f), position);
    // ...
}

// ✅ Explicit for very small accessors
[[nodiscard]] int getActiveCount() const noexcept { return activeCount; }
```

---

## Math & Graphics

### GLM for All Math
```cpp
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

glm::vec3 position;
glm::mat4 transform;
glm::quat rotation;
```

### Column-Major Matrices (GLM Default)
```cpp
// ✅ GLM default matches OpenGL
glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
model = glm::rotate(model, angle, axis);
model = glm::scale(model, scale);
```

### Coordinate Systems
```cpp
// 3D World: Right-handed, Y-up (or Y-down, be consistent)
glm::vec3 worldPos(1.0f, 0.2f, -5.0f);

// 2D Screen: Origin at bottom-left (OpenGL default)
// OR top-left (UI frameworks like Clay)
glm::vec2 screenPos = glm::project(worldPos, camera, projection, viewport);

// If UI uses top-left Y, flip:
float uiY = screenHeight - screenPos.y;
```

---

## Code Organization

### Section Comments
```cpp
// === Configuration ===
static inline const int MAX_COINS = 10;

// === Data ===
std::array<Coin, MAX_COINS> coins{};

// === Initialization ===
void initStars(CoinPattern pattern, int count);

// === Per-Frame Update ===
void updateStars(float deltaTime);

// === Queries ===
[[nodiscard]] int getActiveCount() const noexcept;
```

### Inline Comments
```cpp
// ✅ Explain WHY, not WHAT
coin.flyTriggered = false;  // Allow fly animation to trigger

// ✅ Use emojis for visual scanning (optional)
// 👇 NEW: Try to spawn fly animation immediately
// ⚠️ Important: Don't hide coin here
// ✅ Fixed: removed erroneous 10x multiplier

// ❌ Don't state the obvious
// coin.scale = 0.0f;  // Set scale to zero
```

### Function Grouping
```cpp
struct CoinLane {
    // 1. Constants/config first
    static inline const int MAX_COINS = 10;
    
    // 2. Public data
    std::array<Coin, MAX_COINS> coins{};
    
    // 3. Constructor/Init
    void initStars(...);
    
    // 4. Per-frame updates
    void updateStars(...);
    
    // 5. Queries (const noexcept)
    [[nodiscard]] int getActiveCount() const noexcept;
    
    // 6. Private helpers
private:
    void applyPattern(Coin& c, float globalTime);
};
```

---

## Error Handling

### No Exceptions in Game Loop
```cpp
// ✅ Use assertions for invariants
assert(coinIndex >= 0 && coinIndex < MAX_COINS);

// ✅ Use optional returns for expected failures
[[nodiscard]] bool spawnFlyAnimation(...) noexcept {
    for (auto& anim : flyAnimations) {
        if (!anim.active) {
            anim.start(startPos, targetPos);
            return true;
        }
    }
    return false;  // Pool exhausted - caller handles gracefully
}

// ❌ Avoid try/catch in performance-critical code
// try { ... } catch (...) { }
```

### Debug Logging
```cpp
// ✅ Use std::cerr for debug output (temporary)
std::cerr << "pickup detected" << std::endl;

// TODO: Replace with proper logging system
// Logger::debug("Coin collected at position: {}", coin.position);
```

---

## State Machines

### Explicit Enum-Based States
```cpp:
enum class CoinState : uint8_t {
    Active,      // Moving in world, can be collected
    Collected,   // Picked up, waiting for fly animation
    Dead         // Removed from simulation
};

// ✅ Clear transitions
if (coin.state == CoinState::Active) {
    if (collisionDetected) {
        coin.state = CoinState::Collected;
    }
}

if (coin.state == CoinState::Collected && !coin.flyTriggered) {
    spawnFlyAnimation();
    coin.flyTriggered = true;
}
```

### Avoid Implicit States
```cpp
// ❌ Don't use magic numbers or booleans for state
// bool isCollected;
// bool isFlying;
// int animationFrame;

// ✅ Use explicit state enum
CoinState state;
```

---

## Hot-Reload Design

### All Config in `static inline const`
```cpp
struct CoinFlyConfig {
    static inline const float FLY_DURATION = 0.8f;    // Tunable
    static inline const float ARC_HEIGHT = 0.0f;       // Tunable
    static inline const float PIXEL_SIZE = 40.0f;      // Tunable
};
```

### No Static Initialization Dependencies
```cpp
// ✅ Safe: plain old data (POD)
static inline const float MAX_SPEED = 10.0f;

// ❌ Avoid: complex initialization order issues
// static inline const std::string CONFIG_PATH = getConfigPath();
```

### Interface Stability
```cpp
// ✅ Keep public API stable across reloads
// Don't change method signatures or struct layouts mid-game

// ❌ Avoid: changing struct member order
// struct Coin { float x; float y; };  // v1
// struct Coin { float y; float x; };  // v2 - BREAKS existing state
```

---

## Testing & Debugging

### Constexpr for Compile-Time Checks
```cpp
// ✅ Compile-time validation where possible
static_assert(MAX_COINS > 0, "Must have at least one coin slot");
static_assert(sizeof(Coin) <= 128, "Coin struct too large for cache");
```

### Runtime Assertions
```cpp
// ✅ Validate invariants in debug
void updateStars(float deltaTime) {
    assert(deltaTime >= 0.0f && "Delta time cannot be negative");
    assert(activeCount <= MAX_COINS && "Active count exceeds maximum");
    // ...
}
```

### Profiling Hooks
```cpp
// ✅ Leave room for profiling
void updateStars(float deltaTime) {
    // PROFILE_SCOPE("CoinLane::updateStars");  // Uncomment when profiling
    // ...
}
```

---

## Documentation

### Doxygen-Style Comments (Optional)
```cpp
/// @brief Spawn a new fly animation
/// @param startPos Screen position where coin was collected
/// @param targetPos HUD destination position
/// @return true if animation spawned, false if pool exhausted
/// @note Thread-unsafe: call from main thread only
[[nodiscard]] bool spawnFlyAnimation(
    const glm::vec2& startPos, 
    const glm::vec2& targetPos) noexcept;
```

### In-Code Examples
```cpp
// Example usage:
//   if (coinLane.spawnFlyAnimation(screenPos, hudTarget)) {
//       coin.flyTriggered = true;
//       sound.playCoinPickup();
//   }
```

---

## Build System

### Compiler Flags (GCC/Clang)
```bash
-std=c++17
-O2                    # or -O3 for release, -O0 -g for debug
-Wall -Wextra          # Enable warnings
-Wpedantic             # Strict standard compliance
-Wno-unused-parameter  # Optional: suppress unused param warnings
-fno-omit-frame-pointer # Better stack traces
```

### Compiler Flags (MSVC)
```bash
/std:c++17
/O2                    # Release optimization
/W4                    # Warning level 4
/permissive-           # Strict standard conformance
```

### Include Paths
```bash
-I/path/to/glm         # GLM header-only library
-I/path/to/clay        # Clay UI library
-I./src                # Your project headers
```

---

## Dependencies

### Required
- **GLM** (OpenGL Mathematics): Header-only math library
- **C++17 Standard Library**: `<array>`, `<algorithm>`, `<cmath>`

### Optional
- **Clay UI**: For HUD layout (or any immediate-mode UI)
- **OpenGL**: For rendering (or your preferred graphics API)

---

## Code Review Checklist

Before committing code, verify:

- [ ] All methods marked `noexcept` if they don't throw
- [ ] Query methods marked `[[nodiscard]]`
- [ ] Const-correctness applied (const methods, const refs)
- [ ] No raw `new`/`delete` in game loop
- [ ] Timestep-independent logic (no frame counters)
- [ ] DeltaTime capped reasonably (0.05f max)
- [ ] Config values use `static inline const`
- [ ] Section comments organize code clearly
- [ ] No magic numbers (use named constants)
- [ ] Assertions for invariants
- [ ] No exceptions in hot path
- [ ] Structs used instead of classes (unless encapsulation needed)
- [ ] `enum class` for state machines
- [ ] `std::array` for fixed-size collections

---

## Example: Complete Implementation

```cpp
#pragma once

#include <glm/glm.hpp>
#include <array>
#include <algorithm>

struct CoinConfig {
    static inline const int MAX_COINS = 10;
    static inline const float PICKUP_RADIUS = 0.25f;
    static inline const float ROTATION_SPEED = 2.5f;
};

enum class CoinState : uint8_t {
    Active,
    Collected,
    Dead
};

struct Coin {
    glm::vec3 position{};
    float rotation = 0.0f;
    float scale = 1.0f;
    CoinState state = CoinState::Dead;
    bool flyTriggered = false;
    
    void updateTransform() noexcept {
        // ... implementation
    }
    
    [[nodiscard]] bool isRenderable() const noexcept {
        return state == CoinState::Active && scale > 0.05f;
    }
};

struct CoinLane {
    std::array<Coin, CoinConfig::MAX_COINS> coins{};
    int activeCount = 0;
    
    void initStars(int count) {
        activeCount = std::clamp(count, 0, CoinConfig::MAX_COINS);
        // ... initialization
    }
    
    void updateStars(const glm::vec3& ballPos, float deltaTime) {
        for (int i = 0; i < activeCount; ++i) {
            Coin& c = coins[i];
            if (c.state != CoinState::Active) continue;
            
            // Collision check
            const float distSq = glm::dot(ballPos - c.position, ballPos - c.position);
            if (distSq < CoinConfig::PICKUP_RADIUS * CoinConfig::PICKUP_RADIUS) {
                c.state = CoinState::Collected;
            }
            
            // Update rotation (timestep-independent)
            c.rotation += CoinConfig::ROTATION_SPEED * deltaTime;
            c.updateTransform();
        }
    }
    
    [[nodiscard]] int getActiveCount() const noexcept {
        return activeCount;
    }
    
    [[nodiscard]] const std::array<Coin, CoinConfig::MAX_COINS>& getCoins() const noexcept {
        return coins;
    }
};
```

---

## Version History

- **v1.0** (2026): Initial style guide based on CoinLane implementation
  - C++17 modern features
  - Timestep-independent design
  - Hot-reload friendly architecture

---

## References

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [GLM Documentation](https://glm.g-truc.net/)
- [CppCon Talks on Performance](https://www.youtube.com/user/CppCon)

---

*Last updated: April 2026*
```