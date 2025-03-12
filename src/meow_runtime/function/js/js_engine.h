#pragma once

#include "quickjs.h"

namespace Meow
{
    class JSEngine
    {
    public:
        JSEngine()
        {
            rt  = JS_NewRuntime();
            ctx = JS_NewContext(rt);
        }

        ~JSEngine()
        {
            JS_FreeContext(ctx);
            JS_FreeRuntime(rt);
        }

        void test();

    private:
        JSRuntime* rt;
        JSContext* ctx;
    };
} // namespace Meow