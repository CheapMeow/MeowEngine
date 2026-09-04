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
        // Clear the busy flag and wake the engine thread waiting in
        // WaitForJSIdle(). Called on the JS thread after each command.
        void FinishCommand();

        // --- JS-thread-only helpers (must not be called from other threads) ---

        // Evaluate `code` immediately on the calling (JS) thread. Used both by
        // the command loop and to bootstrap the generated type bindings before
        // any queued script runs.
        void EvalOnJSThread(const std::string& code, const std::string& filename);
        // Log the pending exception (message + stack) and clear it.
        void LogJSException(const std::string& where);

        // Unblock a JS thread that is parked in JS_DebugServerAttach()'s
        // accept(). The DAP server exposes no cancellation API, so we nudge it
        // with a throw-away loopback connection. Safe to call from any thread.
        void WakeDebugServerAccept();

        // Block the calling (engine) thread until the JS thread has drained
        // every queued command. Must be called with `lock` held on m_cmdMutex.
        //
        // JS bridge callbacks mutate live engine state (levels, game objects,
        // resources) straight from the JS thread, so JS execution and the
        // engine's own frame work must never overlap. Handing each command over
        // synchronously keeps exactly one of the two threads inside engine data
        // at any time.
        void WaitForJSIdle(std::unique_lock<std::mutex>& lock);

        JSRuntime*     m_rt  = nullptr;
        JSContext*     m_ctx = nullptr;
        JSDebugServer* m_dapServer = nullptr;  // non-null only when MEOW_DEBUG set

        std::thread m_jsThread;

        std::mutex              m_cmdMutex;
        std::condition_variable m_cmdCv;   // engine -> JS thread (work queued)
        std::condition_variable m_doneCv;  // JS thread -> engine (work finished)
        std::queue<JSCommand>   m_cmdQueue;
        bool                    m_busy = false;  // guarded by m_cmdMutex


        std::atomic<bool> m_shutdown{false};
        std::atomic<bool> m_jsThreadDone{false};
        std::atomic<bool> m_hasUpdate{false};     // a global `update(dt)` exists
        std::atomic<bool> m_dapWaiting{false};    // JS thread parked in accept()
        std::atomic<int>  m_dapPort{0};           // port the DAP server listens on
    };
} // namespace Meow
