#pragma once

#include "core/uuid/uuid.h"
#include "function/components/camera/camera_3d_component.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <quickjs.h>

#include <string>

namespace Meow
{

    template<typename T>
    struct JSConvert;

    // float
    template<>
    struct JSConvert<float>
    {
        static JSValue ToJS(JSContext* ctx, float v) { return JS_NewFloat64(ctx, v); }
        static float   FromJS(JSContext* ctx, JSValue v)
        {
            double d;
            JS_ToFloat64(ctx, &d, v);
            return static_cast<float>(d);
        }
    };

    // double
    template<>
    struct JSConvert<double>
    {
        static JSValue ToJS(JSContext* ctx, double v) { return JS_NewFloat64(ctx, v); }
        static double  FromJS(JSContext* ctx, JSValue v)
        {
            double d;
            JS_ToFloat64(ctx, &d, v);
            return d;
        }
    };

    // int32_t
    template<>
    struct JSConvert<int32_t>
    {
        static JSValue ToJS(JSContext* ctx, int32_t v) { return JS_NewInt32(ctx, v); }
        static int32_t FromJS(JSContext* ctx, JSValue v)
        {
            int32_t i;
            JS_ToInt32(ctx, &i, v);
            return i;
        }
    };

    // bool
    template<>
    struct JSConvert<bool>
    {
        static JSValue ToJS(JSContext* ctx, bool v) { return JS_NewBool(ctx, v); }
        static bool    FromJS(JSContext* ctx, JSValue v) { return JS_ToBool(ctx, v); }
    };

    // UUID
    template<>
    struct JSConvert<UUID>
    {
        static JSValue ToJS(JSContext* ctx, UUID v)
        {
            return JS_NewFloat64(ctx, static_cast<double>(static_cast<uint64_t>(v)));
        }
        static UUID FromJS(JSContext* ctx, JSValue v)
        {
            double d;
            JS_ToFloat64(ctx, &d, v);
            return UUID(static_cast<uint64_t>(d));
        }
    };

    // std::string
    template<>
    struct JSConvert<std::string>
    {
        static JSValue     ToJS(JSContext* ctx, const std::string& v) { return JS_NewString(ctx, v.c_str()); }
        static std::string FromJS(JSContext* ctx, JSValue v)
        {
            const char* s = JS_ToCString(ctx, v);
            std::string result(s);
            JS_FreeCString(ctx, s);
            return result;
        }
    };

    // glm::vec3
    template<>
    struct JSConvert<glm::vec3>
    {
        static JSValue   ToJS(JSContext* ctx, const glm::vec3& v);
        static glm::vec3 FromJS(JSContext* ctx, JSValue v);
    };

    // glm::quat
    template<>
    struct JSConvert<glm::quat>
    {
        static JSValue   ToJS(JSContext* ctx, const glm::quat& v);
        static glm::quat FromJS(JSContext* ctx, JSValue v);
    };

    // CameraMode
    template<>
    struct JSConvert<CameraMode>
    {
        static JSValue    ToJS(JSContext* ctx, CameraMode v);
        static CameraMode FromJS(JSContext* ctx, JSValue v);
    };

} // namespace Meow
