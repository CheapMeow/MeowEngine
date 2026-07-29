#pragma once

#include "function/system.h"

#include <memory>
#include <quickjs.h>
#include <string>

class DAPServer; // quickjs-debugger (global namespace)

namespace Meow
{
    class JSSystem : public System
    {
    public:
        JSSystem()  = default;
        ~JSSystem();

        void Start() override;
        void Tick(float dt) override;
        void Shutdown() override;

        JSContext* GetContext() const { return m_ctx; }

        void LoadScript(const std::string& file_path);

    private:
        JSRuntime* m_rt  = nullptr;
        JSContext* m_ctx = nullptr;

        // Optional embedded debugger. Non-null only when MEOW_DEBUG is set;
        // it attaches to the runtime/context and listens for a DAP client on
        // the port given by MEOW_DEBUG_PORT (default 9229).
        std::unique_ptr<DAPServer> m_dapServer;
    };
} // namespace Meow
