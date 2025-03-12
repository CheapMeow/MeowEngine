#include "js_engine.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace Meow
{
    void JSEngine::test_cpp_call_js()
    {
        // define global js object
        JSValue global_obj = JS_GetGlobalObject(ctx);

        // load js script
        std::string        js_file = "./sample.js";
        std::ifstream      in(js_file.c_str());
        std::ostringstream sin;
        sin << in.rdbuf();
        std::string script_text = sin.str();

        std::cout << "script text: " << std::endl;
        std::cout << script_text << std::endl;

        // run script
        std::cout << "script run: " << std::endl;
        JSValue script = JS_Eval(ctx, script_text.c_str(), script_text.length(), "sample", JS_EVAL_TYPE_GLOBAL);
        if (!JS_IsException(script))
        {
            int         x    = 7;
            double      y    = 8.9;
            std::string text = "called from cpp";

            JSValue js_x    = JS_NewInt32(ctx, x);
            JSValue js_y    = JS_NewFloat64(ctx, y);
            JSValue js_text = JS_NewString(ctx, text.c_str());
            JSValue js_result;

            JSValue my_func = JS_GetPropertyStr(ctx, global_obj, "my_func");

            if (JS_IsFunction(ctx, my_func))
            {
                JSValue params[] = {js_x, js_y, js_text};

                // call js function
                js_result = JS_Call(ctx, my_func, JS_UNDEFINED, 3, params);

                if (!JS_IsException(js_result))
                {
                    double result = 0.0;
                    JS_ToFloat64(ctx, &result, js_result);
                    std::cout << "my_func result: " << result << std::endl;
                }
                else
                    std::cerr << "JS_Call failed" << std::endl;
            }

            JS_FreeValue(ctx, my_func);
            JS_FreeValue(ctx, js_result);
            JS_FreeValue(ctx, js_text);
            JS_FreeValue(ctx, js_y);
            JS_FreeValue(ctx, js_x);
        }
        else
            std::cerr << "JS_Eval failed" << std::endl;

        // close js runtime and context
        JS_FreeValue(ctx, global_obj);
    }

#define JS_INIT_MODULE js_init_module
#define countof(x)     (sizeof(x) / sizeof((x)[0]))

    // define native variable and function
    const int a = 3;
    const int b = 5;

    static double my_func(int x, double y, const char* text)
    {
        std::cout << "my_func with params: " << x << ", " << y << ", " << text << std::endl;

        double z = x * y + (b - a);
        return z;
    }

    // define quickjs C function
    static JSValue js_my_func(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        std::cout << "js_my_func, argc: " << argc << std::endl;

        if (argc != 3)
            return JS_EXCEPTION;

        int    a = 0;
        double b = 0.0;

        if (JS_ToInt32(ctx, &a, argv[0]))
            return JS_EXCEPTION;

        if (JS_ToFloat64(ctx, &b, argv[1]))
            return JS_EXCEPTION;

        if (!JS_IsString(argv[2]))
            return JS_EXCEPTION;

        const char* text = JS_ToCString(ctx, argv[2]);
        double      z    = my_func(a, b, text);

        std::cout << "a: " << a << ", b: " << b << ", text: " << text << ", z: " << z << std::endl;

        return JS_NewFloat64(ctx, z);
    }

    // define function entry list
    static const JSCFunctionListEntry js_my_funcs[] = {
        JS_CFUNC_DEF("my_func", 3, js_my_func),
    };

    static int js_my_init(JSContext* ctx, JSModuleDef* m)
    {
        return JS_SetModuleExportList(ctx, m, js_my_funcs, countof(js_my_funcs));
    }

    JSModuleDef* JS_INIT_MODULE(JSContext* ctx, const char* module_name)
    {
        JSModuleDef* m;
        m = JS_NewCModule(ctx, module_name, js_my_init);
        if (!m)
            return NULL;
        JS_AddModuleExportList(ctx, m, js_my_funcs, countof(js_my_funcs));
        return m;
    }

    void JSEngine::test_js_call_cpp()
    {
        // define global js object
        JSValue global_obj = JS_GetGlobalObject(ctx);

        // register C++ function to current context
        JSValue func_val = JS_NewCFunction(ctx, js_my_func, "my_func", 1);
        if (JS_IsException(func_val))
            std::cerr << "JS_NewCFunction failed" << std::endl;

        if (JS_DefinePropertyValueStr(ctx, global_obj, "my_func", func_val, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE) <
            0)
            std::cerr << "JS_DefinePropertyValue failed" << std::endl;

        std::string        js_file = "./main.js";
        std::ifstream      in(js_file.c_str());
        std::ostringstream sin;
        sin << in.rdbuf();
        std::string script_text = sin.str();

        std::cout << "script text: " << std::endl;
        std::cout << script_text << std::endl;

        std::cout << "script run: " << std::endl;
        JSValue script = JS_Eval(ctx, script_text.c_str(), script_text.length(), "main", JS_EVAL_TYPE_GLOBAL);

        if (JS_IsException(script))
            std::cerr << "JS_Eval failed" << std::endl;

        // close js runtime and context
        JS_FreeValue(ctx, global_obj);
    }
} // namespace Meow