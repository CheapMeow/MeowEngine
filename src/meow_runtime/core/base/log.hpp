#pragma once

#include <chrono>
#include <format>
#include <iostream>

namespace Meow
{
    class Logger
    {
    public:
        static void info(const std::string& message)
        {
            std::cout << "\033[32m" << "[MeowEngine][" << GetCurrentTimeString() << "] " << message << "\033[0m"
                      << std::endl;
        }

        static void warn(const std::string& message)
        {
            std::cout << "\033[33m" << "[MeowEngine][" << GetCurrentTimeString() << "] " << message << "\033[0m"
                      << std::endl;
        }

        static void error(const std::string& message)
        {
            std::cerr << "\033[31m" << "[MeowEngine][" << GetCurrentTimeString() << "] " << message << "\033[0m"
                      << std::endl;
        }

        static std::string GetCurrentTimeString()
        {
            auto const time = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
            return std::format("{:%Y-%m-%d %X}", time);
        }
    };
} // namespace Meow

// Runtime log macros
#define MEOW_INFO(...)  Meow::Logger::info(std::format(__VA_ARGS__))
#define MEOW_WARN(...)  Meow::Logger::warn(std::format(__VA_ARGS__))
#define MEOW_ERROR(...) Meow::Logger::error(std::format(__VA_ARGS__))

#ifndef MEOW_DEBUG
#    define ASSERT(statement)
#else
#    define ASSERT(statement) assert(statement)
#endif
