#include <csignal>
#include <iostream>
#include <algorithm>
#include <cctype>
#include "Server.hpp"
#include "Logger.hpp"

int main()
{
    try
    {
        Logger::Initialize("logs/server.log", 1024 * 1024 * 100, 10);
        spdlog::info("Creating HTTPServer");
        HTTPServer server("127.0.0.1", 8080);
        spdlog::info("Calling server.start()");
        server.start();
        std::cout << "Enter \"quit\" to stop server." << std::endl;

        std::string command;
        while (true)
        {
            if (!std::getline(std::cin, command))
            {
                spdlog::info("Input stream closed (EOF), stopping server");
                break;
            }
            // Trim whitespace and convert to lowercase
            command.erase(std::remove_if(command.begin(), command.end(), [](unsigned char c) { return std::isspace(c); }), command.end());
            std::transform(command.begin(), command.end(), command.begin(), [](unsigned char c) { return std::tolower(c); });
            spdlog::info("Received command: '{}'", command);
            if (command == "quit")
            {
                // spdlog::info("Quit command received");
                break;
            }
            spdlog::info("Invalid command: '{}', enter 'quit' to stop", command);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "quit command entered. Stopping the web server." << std::endl;
        server.stop();
        spdlog::info("Main thread detected shutdown");
    }
    catch (const std::exception& ex)
    {
        spdlog::error("Server error: {}", ex.what());
        return 1;
    }

    return 0;
}