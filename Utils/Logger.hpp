#pragma once
#include <iostream>
#include <filesystem>
#include <format>
#include <spdlog/async.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

class Logger
{
public:
    static void Initialize(const std::string& logFile, size_t maxFileSize, size_t maxFiles)
    {
        try
        {
            std::filesystem::create_directories("logs");

            auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFile, maxFileSize, maxFiles);
            spdlog::init_thread_pool(8192, 1);
            auto logger = std::make_shared<spdlog::async_logger>(
                "server", sink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
            
            logger->set_level(spdlog::level::info);
            logger->flush_on(spdlog::level::info);

            spdlog::set_default_logger(logger);
            std::cerr << std::format("Logger initialized for {}\n", logFile);
        }
        catch (const std::exception& e)
        {
            std::cerr << std::format("Failed to initialize Logger: {}\n", e.what());
        }
    }
};