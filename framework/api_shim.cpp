#include "boot.h"

extern "C" void init(void *ctxptr)
{
    vtx::VertexContext *ctx = static_cast<vtx::VertexContext *>(ctxptr);
    vtx::init(ctx);
}

extern "C" void loop(void *ctxptr)
{
    vtx::VertexContext *ctx = static_cast<vtx::VertexContext *>(ctxptr);
    vtx::loop(ctx);
}

extern "C" void hang(void *ctxptr)
{
    vtx::VertexContext *ctx = static_cast<vtx::VertexContext *>(ctxptr);
    vtx::hang(ctx);
}

extern "C" void load(void *ctxptr)
{
    vtx::VertexContext *ctx = static_cast<vtx::VertexContext *>(ctxptr);
    vtx::load(ctx);
}
