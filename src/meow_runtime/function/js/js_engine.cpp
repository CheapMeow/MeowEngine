#include "js_engine.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace Meow
{
    void JSEngine::test()
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
} // namespace Meow