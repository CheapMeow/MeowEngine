#include "log.h"

#include <memory>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Meow
{
    std::shared_ptr<spdlog::logger> Log::s_RuntimeLogger;
    std::shared_ptr<spdlog::logger> Log::s_EditorLogger;

    void Log::Init()
    {
        std::vector<spdlog::sink_ptr> logSinks;
        logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("MeowEngine.log", true));

        logSinks[0]->set_pattern("%^[%T] %n: %v%$");
        logSinks[1]->set_pattern("[%T] [%l] %n: %v");

        s_RuntimeLogger = std::make_shared<spdlog::logger>("RUNTIME", begin(logSinks), end(logSinks));
        spdlog::register_logger(s_RuntimeLogger);
        s_RuntimeLogger->set_level(spdlog::level::trace);
        s_RuntimeLogger->flush_on(spdlog::level::trace);

        s_EditorLogger = std::make_shared<spdlog::logger>("EDITOR", begin(logSinks), end(logSinks));
        spdlog::register_logger(s_EditorLogger);
        s_EditorLogger->set_level(spdlog::level::trace);
        s_EditorLogger->flush_on(spdlog::level::trace);
    }

    // These simple functions should have been implemented in class declaration as inline function
    // but now implemented in cpp to avoid inline, in order to
    // avoid error that external code expands these inline functions, which results in
    // external code accesses static members stored in dll
    std::shared_ptr<spdlog::logger>& Log::GetRuntimeLogger() { return s_RuntimeLogger; }
    std::shared_ptr<spdlog::logger>& Log::GetEditorLogger() { return s_EditorLogger; }

} // namespace Meow