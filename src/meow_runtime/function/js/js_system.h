#pragma once

#include "function/system.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include <quickjs.h>
#include <quickjs-debugger-server.h>  // native DAP server, compiled into quickjs-ng's `qjs`

namespace Meow
{
    // JSSystem runs the QuickJS virtual machine on a dedicated thread.
    //
    // The engine's main thread owns this system and drives it through
    // Start()/Tick()/Shutdown(), but every QuickJS call (runtime/context
    // creation, script evaluation, bridge callbacks) happens on the JS thread.
    // This isolates JS: the engine can keep ticking (rendering, input, ...) while
    // JS is paused at a breakpoint served by the embedded DAP server.
    //
    // The DAP server is part of quickjs-ng's `qjs` library. When MEOW_DEBUG is
    // set it listens on TCP (default port 9229, override with MEOW_DEBUG_PORT)
    // so VS Code can attach. The JS thread blocks in JS_DebugServerAttach()
    // until a DAP client finishes the handshake, then runs scripts under the
    // debugger. Without MEOW_DEBUG the JS thread runs immediately with no
    // debugger attached.
    class JSSystem : public System
    {
    public:
        JSSystem()  = default;
        ~JSSystem() override;

        void Start() override;
        void Tick(float dt) override;
        void Shutdown() override;

        // Enqueue a script file for evaluation on the JS thread. Returns
        // immediately; evaluation happens asynchronously in FIFO order.
        void LoadScript(const std::string& file_path);

    private:
        // Commands dispatched from the engine thread to the JS thread.
        enum class JSCommandType { Eval, Call, Shutdown };

        struct JSCommand
        {
            JSCommandType type;
            std::string   code;      // Eval: source text
            std::string   filename;  // Eval: source name for stack traces
            std::string   func;      // Call: global function name
            double        arg = 0.0; // Call: single float argument (e.g. dt)
        };

        void JSThreadMain();
        JSCommand WaitForCommand();

        JSRuntime*     m_rt  = nullptr;
        JSContext*     m_ctx = nullptr;
        JSDebugServer* m_dapServer = nullptr;  // non-null only when MEOW_DEBUG set

        std::thread m_jsThread;

        std::mutex              m_cmdMutex;
        std::condition_variable m_cmdCv;
        std::queue<JSCommand>   m_cmdQueue;

        std::atomic<bool> m_shutdown{false};
        std::atomic<bool> m_jsThreadDone{false};
        std::atomic<bool> m_hasUpdate{false};     // a global `update(dt)` exists
        std::atomic<bool> m_updatePending{false}; // an update() call is in flight
    };
} // namespace Meow
