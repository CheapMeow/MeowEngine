#pragma once

#include "core/base/macro.h"

#include <memory>

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#pragma warning(pop)

namespace Meow
{
    class LIBRARY_API Log
    {
    public:
        static void Init();

        static std::shared_ptr<spdlog::logger>& GetRuntimeLogger();
        static std::shared_ptr<spdlog::logger>& GetEditorLogger();

    private:
        static std::shared_ptr<spdlog::logger> s_RuntimeLogger;
        static std::shared_ptr<spdlog::logger> s_EditorLogger;
    };

} // namespace Meow

// Runtime log macros
#define RUNTIME_TRACE(...)    ::Meow::Log::GetRuntimeLogger()->trace(__VA_ARGS__)
#define RUNTIME_INFO(...)     ::Meow::Log::GetRuntimeLogger()->info(__VA_ARGS__)
#define RUNTIME_WARN(...)     ::Meow::Log::GetRuntimeLogger()->warn(__VA_ARGS__)
#define RUNTIME_ERROR(...)    ::Meow::Log::GetRuntimeLogger()->error(__VA_ARGS__)
#define RUNTIME_CRITICAL(...) ::Meow::Log::GetRuntimeLogger()->critical(__VA_ARGS__)

// Editor log macros
#define EDITOR_TRACE(...)    ::Meow::Log::GetEditorLogger()->trace(__VA_ARGS__)
#define EDITOR_INFO(...)     ::Meow::Log::GetEditorLogger()->info(__VA_ARGS__)
#define EDITOR_WARN(...)     ::Meow::Log::GetEditorLogger()->warn(__VA_ARGS__)
#define EDITOR_ERROR(...)    ::Meow::Log::GetEditorLogger()->error(__VA_ARGS__)
#define EDITOR_CRITICAL(...) ::Meow::Log::GetEditorLogger()->critical(__VA_ARGS__)
