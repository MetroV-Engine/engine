/*
    Created on: 30/03/2026
    Filename: CommandRegistry.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "CommandRegistry.hpp"
#include "core/Logger/Logger.hpp"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <utility>

CommandRegistry::CommandRegistry()
{
    add("help", "help", "List the available commands", [this](const std::vector<std::string> &) {
        for (const Command &command : _commands)
            Logger::instance().info("Console", command.usage + "  -  " + command.description);
    });
    add("echo", "echo <text>", "Print a message", [](const std::vector<std::string> &args) {
        std::string text;
        for (const std::string &arg : args)
            text += (text.empty() ? "" : " ") + arg;
        Logger::instance().info("Console", text);
    });
}

void CommandRegistry::add(std::string name, std::string usage, std::string description, Handler handler)
{
    if (name.empty() || name.find(' ') != std::string::npos)
        throw std::invalid_argument("Invalid command name \"" + name + "\"");
    if (std::ranges::any_of(_commands, [&](const Command &c) { return c.name == name; }))
        throw std::invalid_argument("Command \"" + name + "\" is already registered");
    _commands.push_back({std::move(name), std::move(usage), std::move(description), std::move(handler)});
}

std::vector<std::string> CommandRegistry::split(const std::string &line)
{
    std::vector<std::string> words;
    std::string current;
    bool quoted = false;
    bool hasWord = false;
    for (const char c : line) {
        if (c == '"') {
            quoted = !quoted;
            hasWord = true;
        } else if (c == ' ' && !quoted) {
            if (hasWord)
                words.push_back(current);
            current.clear();
            hasWord = false;
        } else {
            current += c;
            hasWord = true;
        }
    }
    if (hasWord)
        words.push_back(current);
    return words;
}

bool CommandRegistry::execute(const std::string &line)
{
    std::vector<std::string> words = split(line);
    if (words.empty())
        return false;
    Logger &logger = Logger::instance();
    logger.debug("Console", "> " + line);

    const auto it = std::ranges::find(_commands, words[0], &Command::name);
    if (it == _commands.end()) {
        logger.error("Console", "Unknown command \"" + words[0] + "\". Type \"help\" for the list.");
        return false;
    }
    words.erase(words.begin());
    try {
        it->handler(words);
    } catch (const std::exception &e) {
        logger.error("Console", it->name + ": " + e.what() + " (usage: " + it->usage + ")");
        return false;
    }
    return true;
}

std::vector<std::string> CommandRegistry::complete(const std::string &prefix) const
{
    std::vector<std::string> matches;
    for (const Command &command : _commands)
        if (command.name.starts_with(prefix))
            matches.push_back(command.name);
    std::ranges::sort(matches);
    return matches;
}
