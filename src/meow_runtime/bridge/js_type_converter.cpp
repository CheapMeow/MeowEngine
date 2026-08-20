#include "js_type_converter.h"

#include "function/components/camera/camera_3d_component.hpp"

namespace Meow
{

    // glm::vec3
    JSValue JSConvert<glm::vec3>::ToJS(JSContext* ctx, const glm::vec3& v)
    {
        JSValue arr = JS_NewArray(ctx);
        JS_SetPropertyUint32(ctx, arr, 0, JS_NewFloat64(ctx, v.x));
        JS_SetPropertyUint32(ctx, arr, 1, JS_NewFloat64(ctx, v.y));
        JS_SetPropertyUint32(ctx, arr, 2, JS_NewFloat64(ctx, v.z));
        return arr;
    }

    glm::vec3 JSConvert<glm::vec3>::FromJS(JSContext* ctx, JSValue v)
    {
        glm::vec3 result;
        double    d;
        JSValue   e0 = JS_GetPropertyUint32(ctx, v, 0);
        JSValue   e1 = JS_GetPropertyUint32(ctx, v, 1);
        JSValue   e2 = JS_GetPropertyUint32(ctx, v, 2);
        JS_ToFloat64(ctx, &d, e0);
        result.x = static_cast<float>(d);
        JS_ToFloat64(ctx, &d, e1);
        result.y = static_cast<float>(d);
        JS_ToFloat64(ctx, &d, e2);
        result.z = static_cast<float>(d);
        JS_FreeValue(ctx, e0);
        JS_FreeValue(ctx, e1);
        JS_FreeValue(ctx, e2);
        return result;
    }

    // glm::quat
    JSValue JSConvert<glm::quat>::ToJS(JSContext* ctx, const glm::quat& v)
    {
        JSValue arr = JS_NewArray(ctx);
        JS_SetPropertyUint32(ctx, arr, 0, JS_NewFloat64(ctx, v.w));
        JS_SetPropertyUint32(ctx, arr, 1, JS_NewFloat64(ctx, v.x));
        JS_SetPropertyUint32(ctx, arr, 2, JS_NewFloat64(ctx, v.y));
        JS_SetPropertyUint32(ctx, arr, 3, JS_NewFloat64(ctx, v.z));
        return arr;
    }

    glm::quat JSConvert<glm::quat>::FromJS(JSContext* ctx, JSValue v)
    {
        glm::quat result;
        double    d;
        JSValue   e0 = JS_GetPropertyUint32(ctx, v, 0);
        JSValue   e1 = JS_GetPropertyUint32(ctx, v, 1);
        JSValue   e2 = JS_GetPropertyUint32(ctx, v, 2);
        JSValue   e3 = JS_GetPropertyUint32(ctx, v, 3);
        JS_ToFloat64(ctx, &d, e0);
        result.w = static_cast<float>(d);
        JS_ToFloat64(ctx, &d, e1);
        result.x = static_cast<float>(d);
        JS_ToFloat64(ctx, &d, e2);
        result.y = static_cast<float>(d);
        JS_ToFloat64(ctx, &d, e3);
        result.z = static_cast<float>(d);
        JS_FreeValue(ctx, e0);
        JS_FreeValue(ctx, e1);
        JS_FreeValue(ctx, e2);
        JS_FreeValue(ctx, e3);
        return result;
    }

    // CameraMode
    JSValue JSConvert<CameraMode>::ToJS(JSContext* ctx, CameraMode v)
    {
        switch (v)
        {
            case CameraMode::ThirdPerson:
                return JS_NewString(ctx, "ThirdPerson");
            case CameraMode::FirstPerson:
                return JS_NewString(ctx, "FirstPerson");
            case CameraMode::Free:
                return JS_NewString(ctx, "Free");
            default:
                return JS_NewString(ctx, "Invalid");
        }
    }

    CameraMode JSConvert<CameraMode>::FromJS(JSContext* ctx, JSValue v)
    {
        const char* s = JS_ToCString(ctx, v);
        std::string str(s);
        JS_FreeCString(ctx, s);

        if (str == "ThirdPerson")
            return CameraMode::ThirdPerson;
        if (str == "FirstPerson")
            return CameraMode::FirstPerson;
        if (str == "Free")
            return CameraMode::Free;
        return CameraMode::Invalid;
    }

} // namespace Meow
