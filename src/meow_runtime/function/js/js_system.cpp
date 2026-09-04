// Winsock must be pulled in before anything can include <windows.h> (pch.h
// does), otherwise the legacy <winsock.h> gets included first and clashes with
// winsock2. Needed by WakeDebugServerAccept() to unblock the DAP accept().
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#endif

#include "js_system.h"

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

#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <fstream>
#include <glm/glm.hpp>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace Meow
{
    // ---- bridge helpers ----

    // Pointers are still passed as double (Win64 heap pointers are < 2^48, so
    // they fit losslessly in a 53-bit mantissa).
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

    // UUIDs are 64-bit values that exceed double's 53-bit mantissa.
    // We split them into two safe 32-bit integers: { hi, lo }.
    // JS side reconstructs with:  (hi * 0x100000000 + lo)  — but since JS
    // numbers still lose precision there, the bridge functions below treat the
    // pair as an opaque handle and always receive { hi, lo } back unchanged.
    static JSValue uuid_to_js(JSContext* ctx, uint64_t id)
    {
        JSValue obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, obj, "hi", JS_NewInt32(ctx, static_cast<int32_t>(id >> 32)));
        JS_SetPropertyStr(ctx, obj, "lo", JS_NewInt32(ctx, static_cast<int32_t>(id & 0xFFFFFFFFu)));
        return obj;
    }

    static uint64_t uuid_from_js(JSContext* ctx, JSValue v)
    {
        uint32_t hi = 0, lo = 0;
        JSValue jhi = JS_GetPropertyStr(ctx, v, "hi");
        JSValue jlo = JS_GetPropertyStr(ctx, v, "lo");
        int32_t tmp;
        if (!JS_IsException(jhi)) { JS_ToInt32(ctx, &tmp, jhi); hi = static_cast<uint32_t>(tmp); }
        if (!JS_IsException(jlo)) { JS_ToInt32(ctx, &tmp, jlo); lo = static_cast<uint32_t>(tmp); }
        JS_FreeValue(ctx, jhi);
        JS_FreeValue(ctx, jlo);
        return (static_cast<uint64_t>(hi) << 32) | lo;
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
        JS_SetPropertyStr(ctx, obj, "uuid", uuid_to_js(ctx, static_cast<uint64_t>(go_id)));
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

        level->SetMainCameraID(UUID(uuid_from_js(ctx, argv[0])));
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
        JS_SetPropertyStr(ctx, obj, "uuid", uuid_to_js(ctx, static_cast<uint64_t>(model_id)));
        return obj;
    }

    static JSValue js_setModelComponent(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        auto* comp  = static_cast<ModelComponent*>(ptr_from_js(ctx, argv[0]));
        auto* model = static_cast<Model*>(ptr_from_js(ctx, argv[1]));

        UUID model_id = model->uuid();
        auto shared   = g_runtime_context.resource_system->GetResource<Model>(model_id);
        comp->model   = shared;

        comp->material_id = UUID(uuid_from_js(ctx, argv[2]));

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
        // Stored as two 32-bit halves to avoid double precision loss with 64-bit UUIDs.
        static uint64_t s_default_mat_id = 0;
        if (argc == 1)
        {
            s_default_mat_id = uuid_from_js(ctx, argv[0]);
            return JS_UNDEFINED;
        }
        return uuid_to_js(ctx, s_default_mat_id);
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

    namespace
    {
        bool DebugEnabled()
        {
            const char* v = std::getenv("MEOW_DEBUG");
            return v != nullptr && v[0] != '\0';
        }

        int DebugPort()
        {
            if (const char* v = std::getenv("MEOW_DEBUG_PORT"))
                if (*v) return std::atoi(v);
            return 9229;
        }

        bool ReadFile(const std::string& path, std::string& out)
        {
            std::ifstream f(path, std::ios::binary);
            if (!f) return false;
            std::ostringstream ss;
            ss << f.rdbuf();
            out = ss.str();
            return true;
        }
    }

    JSSystem::~JSSystem()
    {
        Shutdown();
    }

    void JSSystem::Start()
    {
        // Spawn the dedicated JS thread. All QuickJS state (runtime, context,
        // bindings) is created and used exclusively on that thread; the engine
        // thread only enqueues commands.
        m_jsThread = std::thread(&JSSystem::JSThreadMain, this);
    }

    void JSSystem::WaitForJSIdle(std::unique_lock<std::mutex>& lock)
    {
        m_doneCv.wait(lock, [this] {
            return (m_cmdQueue.empty() && !m_busy) || m_jsThreadDone.load();
        });
    }

    void JSSystem::Tick(float dt)
    {
        // Drive the JS thread once per engine frame. If the loaded scripts
        // define a global `update(dt)`, call it. This is what makes the JS
        // thread's Tick "triggered by the engine process's Tick".
        //
        // The call is handed over synchronously: script code reaches into live
        // engine state through the MeowNative bridge, so the engine thread must
        // stay out of that state until the JS thread is done. While execution
        // is suspended at a breakpoint this therefore blocks the engine too,
        // which is the usual (and safe) debugger behaviour.
        if (!m_hasUpdate || m_shutdown)
            return;

        JSCommand cmd;
        cmd.type = JSCommandType::Call;
        cmd.func = "update";
        cmd.arg  = static_cast<double>(dt);

        std::unique_lock<std::mutex> lock(m_cmdMutex);
        if (m_jsThreadDone)
            return;
        m_cmdQueue.push(std::move(cmd));
        m_cmdCv.notify_one();
        WaitForJSIdle(lock);
    }

    void JSSystem::LoadScript(const std::string& file_path)
    {
        std::string code;
        if (!ReadFile(file_path, code))
        {
            MEOW_ERROR("JSSystem: failed to read script: {}", file_path);
            return;
        }
        JSCommand cmd;
        cmd.type     = JSCommandType::Eval;
        cmd.code     = std::move(code);
        cmd.filename = file_path;

        std::unique_lock<std::mutex> lock(m_cmdMutex);
        if (m_shutdown || m_jsThreadDone)
            return;
        m_cmdQueue.push(std::move(cmd));
        m_cmdCv.notify_one();
        WaitForJSIdle(lock);
    }

    JSSystem::JSCommand JSSystem::WaitForCommand()
    {
        std::unique_lock<std::mutex> lock(m_cmdMutex);
        m_cmdCv.wait(lock, [this] { return !m_cmdQueue.empty() || m_shutdown.load(); });
        if (m_shutdown && m_cmdQueue.empty())
            return JSCommand{JSCommandType::Shutdown, {}, {}, {}, 0.0};
        JSCommand cmd = std::move(m_cmdQueue.front());
        m_cmdQueue.pop();
        m_busy = true;  // cleared once the command has been executed
        return cmd;
    }

    void JSSystem::FinishCommand()
    {
        {
            std::lock_guard<std::mutex> lock(m_cmdMutex);
            m_busy = false;
        }
        m_doneCv.notify_all();
    }

    void JSSystem::LogJSException(const std::string& where)
    {
        JSValue exc = JS_GetException(m_ctx);

        JSValue     msg = JS_ToString(m_ctx, exc);
        const char* str = JS_ToCString(m_ctx, msg);
        MEOW_ERROR("JSSystem: JS error in {}: {}", where, str ? str : "(no message)");
        if (str)
            JS_FreeCString(m_ctx, str);
        JS_FreeValue(m_ctx, msg);

        // The stack lives on the exception object, so it must be read *before*
        // `exc` is released (reading it afterwards is a use-after-free).
        if (JS_IsObject(exc))
        {
            JSValue stack = JS_GetPropertyStr(m_ctx, exc, "stack");
            if (!JS_IsException(stack) && !JS_IsUndefined(stack))
            {
                if (const char* stack_str = JS_ToCString(m_ctx, stack))
                {
                    std::cerr << stack_str << std::endl;
                    JS_FreeCString(m_ctx, stack_str);
                }
            }
            JS_FreeValue(m_ctx, stack);
        }

        JS_FreeValue(m_ctx, exc);
    }

    void JSSystem::EvalOnJSThread(const std::string& code, const std::string& filename)
    {
        JSValue result = JS_Eval(m_ctx, code.c_str(), code.size(), filename.c_str(), JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(result))
            LogJSException(filename);
        JS_FreeValue(m_ctx, result);

        // Track whether a global `update(dt)` is now available.
        JSValue g   = JS_GetGlobalObject(m_ctx);
        JSValue fn  = JS_GetPropertyStr(m_ctx, g, "update");
        m_hasUpdate = JS_IsFunction(m_ctx, fn);
        JS_FreeValue(m_ctx, fn);
        JS_FreeValue(m_ctx, g);
    }

    void JSSystem::JSThreadMain()
    {
        m_rt  = JS_NewRuntime();
        m_ctx = JS_NewContext(m_rt);

        // Register auto-generated per-type bindings.
        RegisterJSBindingFunctions(m_ctx);

        // Register the MeowNative bridge object used by game scripts.
        JSValue global      = JS_GetGlobalObject(m_ctx);
        JSValue meow_native = JS_NewObject(m_ctx);
        JS_SetPropertyStr(m_ctx, global, "MeowNative", meow_native);
        JS_SetPropertyFunctionList(m_ctx, meow_native, js_meow_native_funcs,
                                   sizeof(js_meow_native_funcs) / sizeof(js_meow_native_funcs[0]));
        JS_FreeValue(m_ctx, global);

        // Load the generated type bindings synchronously, before the command
        // loop starts. Enqueueing them would race with scripts queued by the
        // engine thread (e.g. editor.cpp's init_scene.js), which could then run
        // before the bindings exist.
        {
            std::string bootstrap;
            const std::string bootstrap_path = ENGINE_ROOT_DIR "/src/meow_runtime/generated/js_types.js";
            if (ReadFile(bootstrap_path, bootstrap))
                EvalOnJSThread(bootstrap, bootstrap_path);
            else
                MEOW_ERROR("JSSystem: failed to read script: {}", bootstrap_path);
        }

        // Optional embedded DAP server (compiled into quickjs-ng's `qjs`).
        // Block until a DAP client (VS Code) connects and finishes the
        // handshake, then run scripts under the debugger.
        if (DebugEnabled() && !m_shutdown)
        {
            const int port = DebugPort();
            m_dapServer     = JS_DebugServerInit(m_rt, m_ctx, "127.0.0.1", port);
            if (m_dapServer)
            {
                m_dapPort = port;
                std::printf("MeowEngine JS debugger listening on 127.0.0.1:%d (waiting for attach)\n", port);
                std::fflush(stdout);

                // accept() below is uninterruptible; publish the fact that we
                // are parked in it so Shutdown() can nudge us out.
                m_dapWaiting = true;
                const int attached = JS_DebugServerAttach(m_dapServer);  // blocks for handshake
                m_dapWaiting = false;

                if (attached == 0)
                {
                    // We drive JS_Eval/JS_Call ourselves instead of handing a
                    // single script to JS_DebugServerRun, so the server has to
                    // be told the VM is live — otherwise its interrupt handler
                    // returns early and no breakpoint ever fires.
                    JS_DebugServerSetRunning(m_dapServer, true);
                    std::printf("MeowEngine JS debugger attached\n");
                    std::fflush(stdout);
                }
                else if (!m_shutdown)
                {
                    MEOW_ERROR("JSSystem: DAP handshake failed; continuing without a debugger");
                }
            }
            else
            {
                MEOW_ERROR("JSSystem: failed to start DAP server on port {}", port);
            }
        }

        // Evaluation loop. Commands from the engine thread (script loads,
        // per-frame update calls) are processed in FIFO order. Breakpoints and
        // stepping are handled by the interrupt handler installed by
        // JS_DebugServerInit and pause this thread inside JS_Eval/JS_Call.
        while (!m_shutdown)
        {
            JSCommand cmd = WaitForCommand();
            if (cmd.type == JSCommandType::Shutdown)
                break;

            if (cmd.type == JSCommandType::Eval)
            {
                EvalOnJSThread(cmd.code, cmd.filename);
            }
            else if (cmd.type == JSCommandType::Call)
            {
                JSValue global = JS_GetGlobalObject(m_ctx);
                JSValue fn     = JS_GetPropertyStr(m_ctx, global, cmd.func.c_str());
                if (JS_IsFunction(m_ctx, fn))
                {
                    JSValue arg    = JS_NewFloat64(m_ctx, cmd.arg);
                    JSValue result = JS_Call(m_ctx, fn, JS_UNDEFINED, 1, &arg);
                    if (JS_IsException(result))
                        LogJSException(cmd.func + "()");
                    JS_FreeValue(m_ctx, result);
                    JS_FreeValue(m_ctx, arg);
                }
                JS_FreeValue(m_ctx, fn);
                JS_FreeValue(m_ctx, global);
            }

            FinishCommand();
        }

        if (m_dapServer)
        {
            JS_DebugServerFree(m_dapServer);
            m_dapServer = nullptr;
        }
        JS_FreeContext(m_ctx);
        JS_FreeRuntime(m_rt);
        m_ctx = nullptr;
        m_rt  = nullptr;

        // Release anyone blocked in WaitForJSIdle(): no further commands will
        // ever be executed once this thread is gone.
        {
            std::lock_guard<std::mutex> lock(m_cmdMutex);
            m_busy = false;
            m_jsThreadDone = true;
        }
        m_doneCv.notify_all();
    }

    void JSSystem::WakeDebugServerAccept()
    {
        const int port = m_dapPort.load();
        if (port <= 0)
            return;

        // A throw-away loopback connection makes the pending accept() return.
        // The subsequent DAP handshake sees an immediate EOF and fails, which
        // is exactly what we want: the JS thread falls through to its command
        // loop, observes m_shutdown and exits cleanly.
#if defined(_WIN32)
        WSADATA wsa_data;
        const bool wsa_ok = (WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0);
        SOCKET     sock   = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock != INVALID_SOCKET)
        {
            sockaddr_in addr {};
            addr.sin_family      = AF_INET;
            addr.sin_port        = htons(static_cast<u_short>(port));
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            ::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
            ::closesocket(sock);
        }
        if (wsa_ok)
            WSACleanup();
#else
        int sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sock >= 0)
        {
            sockaddr_in addr {};
            addr.sin_family      = AF_INET;
            addr.sin_port        = htons(static_cast<uint16_t>(port));
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            ::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
            ::close(sock);
        }
#endif
    }

    void JSSystem::Shutdown()
    {
        if (m_shutdown.exchange(true))
            return;  // already shutting down
        {
            std::unique_lock<std::mutex> lock(m_cmdMutex);
            m_cmdQueue.push(JSCommand{JSCommandType::Shutdown, {}, {}, {}, 0.0});
        }
        m_cmdCv.notify_one();

        // If the JS thread never got a debugger attached it is stuck inside
        // accept(), where neither the shutdown flag nor the condition variable
        // can reach it. Nudge it so join() below cannot hang forever.
        if (m_dapWaiting)
            WakeDebugServerAccept();

        if (m_jsThread.joinable())
            m_jsThread.join();
    }

} // namespace Meow
