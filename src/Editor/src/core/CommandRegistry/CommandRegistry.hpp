/*
    Created on: 30/03/2026
    Filename: CommandRegistry.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include <functional>
#include <string>
#include <vector>

// Commands typed in the developer console, e.g. "theme Light" or "vsync off".
// Each command gets its arguments already split (quotes group words:
// echo "hello world"). Output goes through the Logger, category "Console".
class CommandRegistry
{
public:
    using Handler = std::function<void(const std::vector<std::string> &args)>;

    struct Command
    {
        std::string name;
        std::string usage; // e.g. "theme <name>"
        std::string description;
        Handler handler;
    };

    CommandRegistry();

    // Throws std::invalid_argument if the name is empty, has spaces or is taken.
    void add(std::string name, std::string usage, std::string description, Handler handler);
    // Parses and runs a command line; unknown commands and handler exceptions
    // are logged as errors. Returns false if the command did not run successfully.
    bool execute(const std::string &line);

    const std::vector<Command> &commands() const { return _commands; }
    // Command names starting with prefix, sorted.
    std::vector<std::string> complete(const std::string &prefix) const;

    // Splits a command line into words, honouring double quotes.
    static std::vector<std::string> split(const std::string &line);

private:
    std::vector<Command> _commands;
};
