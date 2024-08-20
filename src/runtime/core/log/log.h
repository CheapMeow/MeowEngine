#pragma once

#include "core/base/macro.h"

#include <format>
#include <memory>


// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#pragma warning(pop)

namespace Meow
{
    /**
     * @brief Log using spdlog, sperate runtime logger from editor logger.
     */
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
#define RUNTIME_TRACE(...)    ::Meow::Log::GetRuntimeLogger()->trace(std::format(__VA_ARGS__))
#define RUNTIME_INFO(...)     ::Meow::Log::GetRuntimeLogger()->info(std::format(__VA_ARGS__))
#define RUNTIME_WARN(...)     ::Meow::Log::GetRuntimeLogger()->warn(std::format(__VA_ARGS__))
#define RUNTIME_ERROR(...)    ::Meow::Log::GetRuntimeLogger()->error(std::format(__VA_ARGS__))
#define RUNTIME_CRITICAL(...) ::Meow::Log::GetRuntimeLogger()->critical(std::format(__VA_ARGS__))

// Editor log macros
#define EDITOR_TRACE(...)    ::Meow::Log::GetEditorLogger()->trace(std::format(__VA_ARGS__))
#define EDITOR_INFO(...)     ::Meow::Log::GetEditorLogger()->info(std::format(__VA_ARGS__))
#define EDITOR_WARN(...)     ::Meow::Log::GetEditorLogger()->warn(std::format(__VA_ARGS__))
#define EDITOR_ERROR(...)    ::Meow::Log::GetEditorLogger()->error(std::format(__VA_ARGS__))
#define EDITOR_CRITICAL(...) ::Meow::Log::GetEditorLogger()->critical(std::format(__VA_ARGS__))

#ifndef MEOW_DEBUG
#    define ASSERT(statement)
#else
#    define ASSERT(statement) assert(statement)
#endif