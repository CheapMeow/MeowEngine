#include "js_system.h"

#include "DAPServer.h"
#include "bridge/js_type_converter.h"
#include "core/reflect/reflect.hpp"
#include "function/components/camera/camera_3d_component.hpp"
#include "function/components/light/directional_light_component.h"
#include "function/components/model/model_component.h"
#include "function/components/transform/transform_3d_component.hpp"
#include "function/global/runtime_context.h"
#include "function/render/geometry/geometry_factory.h"
#include "function/render/model/model.hpp"
#include "generated/register_js_binding.h"
#include "pch.h"

#include <cstdlib>
#include <fstream>
#include <glm/glm.hpp>
#include <sstream>

namespace Meow
{
    // ---- bridge helpers ----

    static void* ptr_from_js(JSContext* ctx, JSValue v)
    {
        double d;
        JS_ToFloat64(ctx, &d, v);
        return reinterpret_cast<void*>(static_cast<uint64_t>(d));
    }

    static JSValue ptr_to_js(JSContext* ctx, void* ptr)
    {
        return JS_NewFloat64(ctx, static_cast<double>(reinterpret_cast<uint64_t>(ptr)));
    }

    // ---- native bridge functions ----

    static JSValue js_createObject(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        auto level = g_runtime_context.level_system->GetCurrentActiveLevel().lock();
        if (!level)
            return JS_ThrowTypeError(ctx, "No active level");

        UUID go_id = level->CreateObject();
        auto go    = level->GetGameObjectByID(go_id).lock();
        if (!go)
            return JS_ThrowTypeError(ctx, "Failed to create object");

        JSValue obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, obj, "uuid", JS_NewFloat64(ctx, static_cast<double>(static_cast<uint64_t>(go_id))));
        JS_SetPropertyStr(ctx, obj, "ptr", ptr_to_js(ctx, go.get()));
        return obj;
    }

    static JSValue js_addComponent(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        if (argc < 2)
            return JS_ThrowTypeError(ctx, "addComponent requires 2 args");

        GameObject* go            = static_cast<GameObject*>(ptr_from_js(ctx, argv[0]));
        const char* type_name_str = JS_ToCString(ctx, argv[1]);
        std::string type_name(type_name_str);
        JS_FreeCString(ctx, type_name_str);

        // use reflection to create instance
        if (!reflect::Registry::instance().HasType(type_name))
            return JS_ThrowTypeError(ctx, "Unknown component type: %s", type_name.c_str());

        std::shared_ptr<Component> comp;

        if (type_name == "Transform3DComponent")
            comp = std::make_shared<Transform3DComponent>();
        else if (type_name == "Camera3DComponent")
            comp = std::make_shared<Camera3DComponent>();
        else if (type_name == "DirectionalLightComponent")
            comp = std::make_shared<DirectionalLightComponent>();
        else if (type_name == "ModelComponent")
            comp = std::make_shared<ModelComponent>();

        if (!comp)
            return JS_ThrowTypeError(ctx, "Component creation not supported for this type");

        auto go_shared =
            g_runtime_context.level_system->GetCurrentActiveLevel().lock()->GetGameObjectByID(go->GetID()).lock();
        if (!go_shared)
            return JS_ThrowTypeError(ctx, "GameObject no longer valid");

        auto added = TryAddComponent(go_shared, type_name, comp);
        if (!added)
            return JS_ThrowTypeError(ctx, "Failed to add component");

        JSValue obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, obj, "type_name", JS_NewString(ctx, type_name.c_str()));
        JS_SetPropertyStr(ctx, obj, "ptr", ptr_to_js(ctx, added.get()));
        return obj;
    }

    static JSValue js_setMainCameraID(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        auto level = g_runtime_context.level_system->GetCurrentActiveLevel().lock();
        if (!level)
            return JS_ThrowTypeError(ctx, "No active level");

        double uuid_val;
        JS_ToFloat64(ctx, &uuid_val, argv[0]);
        level->SetMainCameraID(UUID(static_cast<uint64_t>(uuid_val)));
        return JS_UNDEFINED;
    }

    static JSValue js_setName(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        GameObject* go   = static_cast<GameObject*>(ptr_from_js(ctx, argv[0]));
        const char* name = JS_ToCString(ctx, argv[1]);
        go->SetName(name);
        JS_FreeCString(ctx, name);
        return JS_UNDEFINED;
    }

    static JSValue js_createCubeMesh(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        GeometryFactory gf;
        gf.SetCube();

        // Read vertex attributes from argv[0] (array of strings)
        std::vector<VertexAttributeBit> attrs;
        JSValue                         attr_arr = argv[0];
        if (JS_IsArray(attr_arr))
        {
            for (int i = 0;; i++)
            {
                JSValue elem = JS_GetPropertyUint32(ctx, attr_arr, i);
                if (JS_IsUndefined(elem))
                {
                    JS_FreeValue(ctx, elem);
                    break;
                }
                const char* s = JS_ToCString(ctx, elem);
                attrs.push_back(to_enum(std::string(s)));
                JS_FreeCString(ctx, s);
                JS_FreeValue(ctx, elem);
            }
        }

        auto vertices = gf.GetVertices(attrs);
        auto indices  = gf.GetIndices();

        JSValue result      = JS_NewObject(ctx);
        JSValue js_vertices = JS_NewArray(ctx);
        for (size_t i = 0; i < vertices.size(); i++)
            JS_SetPropertyUint32(ctx, js_vertices, i, JS_NewFloat64(ctx, vertices[i]));
        JS_SetPropertyStr(ctx, result, "vertices", js_vertices);

        JSValue js_indices = JS_NewArray(ctx);
        for (size_t i = 0; i < indices.size(); i++)
            JS_SetPropertyUint32(ctx, js_indices, i, JS_NewInt32(ctx, indices[i]));
        JS_SetPropertyStr(ctx, result, "indices", js_indices);

        return result;
    }

    static JSValue js_createPlaneMesh(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        GeometryFactory gf;
        gf.SetPlane();

        std::vector<VertexAttributeBit> attrs;
        JSValue                         attr_arr = argv[0];
        if (JS_IsArray(attr_arr))
        {
            for (int i = 0;; i++)
            {
                JSValue elem = JS_GetPropertyUint32(ctx, attr_arr, i);
                if (JS_IsUndefined(elem))
                {
                    JS_FreeValue(ctx, elem);
                    break;
                }
                const char* s = JS_ToCString(ctx, elem);
                attrs.push_back(to_enum(std::string(s)));
                JS_FreeCString(ctx, s);
                JS_FreeValue(ctx, elem);
            }
        }

        auto vertices = gf.GetVertices(attrs);
        auto indices  = gf.GetIndices();

        JSValue result      = JS_NewObject(ctx);
        JSValue js_vertices = JS_NewArray(ctx);
        for (size_t i = 0; i < vertices.size(); i++)
            JS_SetPropertyUint32(ctx, js_vertices, i, JS_NewFloat64(ctx, vertices[i]));
        JS_SetPropertyStr(ctx, result, "vertices", js_vertices);

        JSValue js_indices = JS_NewArray(ctx);
        for (size_t i = 0; i < indices.size(); i++)
            JS_SetPropertyUint32(ctx, js_indices, i, JS_NewInt32(ctx, indices[i]));
        JS_SetPropertyStr(ctx, result, "indices", js_indices);

        return result;
    }

    static JSValue js_createModel(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        // argv[0]: vertices JS array, argv[1]: indices JS array, argv[2]: attribute names array
        std::vector<float>              verts;
        std::vector<uint32_t>           inds;
        std::vector<VertexAttributeBit> attrs;

        JSValue v_arr = argv[0];
        for (int i = 0;; i++)
        {
            JSValue elem = JS_GetPropertyUint32(ctx, v_arr, i);
            if (JS_IsUndefined(elem))
            {
                JS_FreeValue(ctx, elem);
                break;
            }
            double d;
            JS_ToFloat64(ctx, &d, elem);
            verts.push_back(static_cast<float>(d));
            JS_FreeValue(ctx, elem);
        }

        JSValue i_arr = argv[1];
        for (int i = 0;; i++)
        {
            JSValue elem = JS_GetPropertyUint32(ctx, i_arr, i);
            if (JS_IsUndefined(elem))
            {
                JS_FreeValue(ctx, elem);
                break;
            }
            int32_t v;
            JS_ToInt32(ctx, &v, elem);
            inds.push_back(static_cast<uint32_t>(v));
            JS_FreeValue(ctx, elem);
        }

        JSValue a_arr = argv[2];
        for (int i = 0;; i++)
        {
            JSValue elem = JS_GetPropertyUint32(ctx, a_arr, i);
            if (JS_IsUndefined(elem))
            {
                JS_FreeValue(ctx, elem);
                break;
            }
            const char* s = JS_ToCString(ctx, elem);
            attrs.push_back(to_enum(std::string(s)));
            JS_FreeCString(ctx, s);
            JS_FreeValue(ctx, elem);
        }

        auto model    = std::make_shared<Model>(verts, inds, attrs);
        UUID model_id = model->uuid();
        g_runtime_context.resource_system->Register(model);

        JSValue obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, obj, "ptr", ptr_to_js(ctx, model.get()));
        JS_SetPropertyStr(ctx, obj, "uuid", JS_NewFloat64(ctx, static_cast<double>(static_cast<uint64_t>(model_id))));
        return obj;
    }

    static JSValue js_setModelComponent(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        auto* comp  = static_cast<ModelComponent*>(ptr_from_js(ctx, argv[0]));
        auto* model = static_cast<Model*>(ptr_from_js(ctx, argv[1]));

        UUID model_id = model->uuid();
        auto shared   = g_runtime_context.resource_system->GetResource<Model>(model_id);
        comp->model   = shared;

        double mat_id;
        JS_ToFloat64(ctx, &mat_id, argv[2]);
        comp->material_id = UUID(static_cast<uint64_t>(mat_id));

        return JS_UNDEFINED;
    }

    static JSValue js_getDefaultVertexAttributes(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        // Hardcoded for forward pass: [Position, UV0, Normal]
        JSValue arr = JS_NewArray(ctx);
        JS_SetPropertyUint32(ctx, arr, 0, JS_NewString(ctx, "Position"));
        JS_SetPropertyUint32(ctx, arr, 1, JS_NewString(ctx, "UV0"));
        JS_SetPropertyUint32(ctx, arr, 2, JS_NewString(ctx, "Normal"));
        return arr;
    }

    static JSValue js_getDefaultMaterialID(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        // This will be set by EditorWindow after render pass creation.
        // For now, return a flag that the caller can override.
        // Actually, we'll store this in a static that EditorWindow updates.
        static uint64_t s_default_mat_id = 0;
        if (argc == 1)
        {
            double v;
            JS_ToFloat64(ctx, &v, argv[0]);
            s_default_mat_id = static_cast<uint64_t>(v);
            return JS_UNDEFINED;
        }
        return JS_NewFloat64(ctx, static_cast<double>(s_default_mat_id));
    }

    static JSValue js_print(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        for (int i = 0; i < argc; i++)
        {
            JSValue  str = JS_ToString(ctx, argv[i]);
            const char* s = JS_ToCString(ctx, str);
            std::cout << (s ? s : "") << (i + 1 < argc ? " " : "");
            if (s) JS_FreeCString(ctx, s);
            JS_FreeValue(ctx, str);
        }
        std::cout << std::endl;
        return JS_UNDEFINED;
    }

    static const JSCFunctionListEntry js_meow_native_funcs[] = {
        JS_CFUNC_DEF("print", 0, js_print),
        JS_CFUNC_DEF("createObject", 0, js_createObject),
        JS_CFUNC_DEF("addComponent", 2, js_addComponent),
        JS_CFUNC_DEF("setMainCameraID", 1, js_setMainCameraID),
        JS_CFUNC_DEF("setName", 2, js_setName),
        JS_CFUNC_DEF("createCubeMesh", 1, js_createCubeMesh),
        JS_CFUNC_DEF("createPlaneMesh", 1, js_createPlaneMesh),
        JS_CFUNC_DEF("createModel", 3, js_createModel),
        JS_CFUNC_DEF("setModelComponent", 3, js_setModelComponent),
        JS_CFUNC_DEF("getDefaultVertexAttributes", 0, js_getDefaultVertexAttributes),
        JS_CFUNC_DEF("getDefaultMaterialID", 0, js_getDefaultMaterialID),
    };

    // ---- JSSystem ----

    JSSystem::~JSSystem() = default;

    void JSSystem::Start()
    {
        m_rt  = JS_NewRuntime();
        m_ctx = JS_NewContext(m_rt);

        // register auto-generated per-type bindings
        RegisterJSBindingFunctions(m_ctx);

        // register MeowNative bridge functions
        JSValue global      = JS_GetGlobalObject(m_ctx);
        JSValue meow_native = JS_NewObject(m_ctx);
        JS_SetPropertyStr(m_ctx, global, "MeowNative", meow_native);

        JS_SetPropertyFunctionList(m_ctx, meow_native,
            js_meow_native_funcs, sizeof(js_meow_native_funcs) / sizeof(js_meow_native_funcs[0]));

        // load generated JS types
        LoadScript(ENGINE_ROOT_DIR "/src/meow_runtime/generated/js_types.js");

        JS_FreeValue(m_ctx, global);

        // Optional embedded QuickJS debugger. Only active when MEOW_DEBUG is
        // set, so production runs pay no interrupt-handler overhead. The DAP
        // server attaches to our runtime/context and listens on a TCP port;
        // VS Code connects to it (attach request) from the extension.
        if (std::getenv("MEOW_DEBUG"))
        {
            m_dapServer = std::make_unique<::DAPServer>(m_rt, m_ctx);

            int port = 9229;
            if (const char* portEnv = std::getenv("MEOW_DEBUG_PORT"))
                port = std::atoi(portEnv);

            if (!m_dapServer->Listen(port))
                MEOW_ERROR("QuickJS debugger failed to listen on port {}", port);
            else
                MEOW_INFO("QuickJS debugger listening on port {}", port);
        }
    }

    void JSSystem::Tick(float dt)
    {
        // Service the embedded debugger. In TCP mode this is a no-op because the
        // DAP server owns its own reader/monitor threads; the hook exists so a
        // host tick can safely drive message pumping if the transport ever
        // needs it.
        if (m_dapServer)
            m_dapServer->Pump();
    }

    void JSSystem::Shutdown()
    {
        if (m_dapServer)
        {
            m_dapServer->Shutdown();
            m_dapServer = nullptr;
        }
        if (m_ctx)
        {
            JS_FreeContext(m_ctx);
            m_ctx = nullptr;
        }
        if (m_rt)
        {
            JS_FreeRuntime(m_rt);
            m_rt = nullptr;
        }
    }

    void JSSystem::LoadScript(const std::string& file_path)
    {
        if (!m_ctx)
        {
            MEOW_ERROR("JSSystem not started");
            return;
        }

        std::ifstream in(file_path);
        if (!in)
        {
            MEOW_ERROR("Failed to open script: {}", file_path);
            return;
        }

        std::ostringstream sin;
        sin << in.rdbuf();
        std::string script_text = sin.str();

        JSValue script =
            JS_Eval(m_ctx, script_text.c_str(), script_text.length(), file_path.c_str(), JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(script))
        {
            JSValue exception = JS_GetException(m_ctx);
            JSValue msg       = JS_ToString(m_ctx, exception);
            const char* str   = JS_ToCString(m_ctx, msg);
            MEOW_ERROR("JS error in {}: {}", file_path, str ? str : "(no message)");
            if (str) JS_FreeCString(m_ctx, str);
            JS_FreeValue(m_ctx, msg);
            JS_FreeValue(m_ctx, exception);

            // print stack if available
            JSValue stack = JS_GetPropertyStr(m_ctx, exception, "stack");
            if (JS_IsException(stack))
            {
                JS_FreeValue(m_ctx, stack);
            }
            else
            {
                const char* stack_str = JS_ToCString(m_ctx, stack);
                if (stack_str)
                {
                    std::cerr << stack_str << std::endl;
                    JS_FreeCString(m_ctx, stack_str);
                }
                JS_FreeValue(m_ctx, stack);
            }
        }

        JS_FreeValue(m_ctx, script);
    }

} // namespace Meow
