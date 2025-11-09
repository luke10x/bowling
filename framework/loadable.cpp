#include "ctx.h"

namespace vtx {
    void init(VertexContext* ctx);
    void loop(VertexContext* ctx);
}

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