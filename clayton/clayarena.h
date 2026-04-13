// =============================================================================
// clay_arena.h — Self-contained bump allocator for Clay UI strings
// - Embedded buffer (no external allocation needed)
// - Sets isStaticallyAllocated = false for arena strings (your Clay version)
// - C-compatible, zero malloc/free, Emscripten-safe
// =============================================================================
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <clay.h>

// -----------------------------------------------------------------------------
// Configuration — embedded buffer size (tune as needed)
// -----------------------------------------------------------------------------
#define CLAY_ARENA_CAPACITY (256 * 1024)  // 256KB embedded in each UserContext

// -----------------------------------------------------------------------------
// Arena struct — embed directly in UserContext
// Buffer is INSIDE the struct (no external pointer needed)
// -----------------------------------------------------------------------------
typedef struct {
    uint8_t buffer[CLAY_ARENA_CAPACITY];  // ← Embedded buffer
    size_t  offset;                        // Current bump offset
} ClayArena;

// -----------------------------------------------------------------------------
// Initialize arena — just reset offset (call once at startup)
// Since buffer is embedded, no external allocation needed
// -----------------------------------------------------------------------------
inline void ClayArena_Init(ClayArena* self) {
    if (!self) return;
    self->offset = 0;
    // buffer is statically zeroed if UserContext is static/global
}

// -----------------------------------------------------------------------------
// Reset for new frame — O(1) rewind (call BEFORE building any Clay UI)
// -----------------------------------------------------------------------------
inline void ClayArena_Reset(ClayArena* self) {
    if (!self) return;
    self->offset = 0;
}

// -----------------------------------------------------------------------------
// Allocate raw bytes from embedded buffer
// Returns NULL if arena is full (handle gracefully)
// -----------------------------------------------------------------------------
inline void* ClayArena_Alloc(ClayArena* self, size_t bytes) {
    if (!self) return NULL;
    if (self->offset + bytes > CLAY_ARENA_CAPACITY) {
        return NULL;  // Overflow — fallback handled by caller
    }
    void* ptr = self->buffer + self->offset;
    self->offset += bytes;
    return ptr;
}

// -----------------------------------------------------------------------------
// Allocate C-string → returns Clay_String with isStaticallyAllocated = false
// This is the KEY function for dynamic text in Clay
// -----------------------------------------------------------------------------
inline Clay_String ClayArena_AllocString(ClayArena* self, const char* cstr) {
    // Handle null/empty input → return static empty string
    if (!cstr || !cstr[0]) {
        return (Clay_String){
            .chars = "",
            .length = 0,
            .isStaticallyAllocated = true,  // ← Static empty string
        };
    }
    
    size_t len = strlen(cstr);
    char* mem = (char*)ClayArena_Alloc(self, len + 1);
    
    // Overflow fallback → return static fallback string
    if (!mem) {
        return (Clay_String){
            .chars = "[OVF]",
            .length = 5,
            .isStaticallyAllocated = true,  // ← Static fallback
        };
    }
    
    // Copy string into arena memory
    memcpy(mem, cstr, len + 1);
    
    // ✅ Return Clay_String with isStaticallyAllocated = false
    // This tells Clay: "this pointer is valid until next arena reset"
    return (Clay_String){
        .chars = mem,
        .length = (int32_t)len,
        .isStaticallyAllocated = false,  // ← KEY: Arena-allocated, not static
    };
}

// -----------------------------------------------------------------------------
// Format string directly into arena (printf-style)
// Returns Clay_String ready for CLAY_TEXT()
// -----------------------------------------------------------------------------
inline Clay_String ClayArena_FormatString(ClayArena* self, const char* format, ...) {
    char temp[256];  // Stack buffer for formatting (safe for UI strings)
    
    va_list args;
    va_start(args, format);
    int len = vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);
    
    // Handle formatting errors
    if (len < 0) len = 0;
    if ((size_t)len >= sizeof(temp)) len = (int)(sizeof(temp) - 1);
    temp[len] = '\0';
    
    // Delegate to AllocString (handles arena allocation + Clay_String construction)
    return ClayArena_AllocString(self, temp);
}