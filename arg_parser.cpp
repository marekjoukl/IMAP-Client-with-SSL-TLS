/**************************
 * IMAP4rev1 client 
 * Author: Marek Joukl
 * Date: 15.11. 2024 
 * Login: xjoukl00
**************************/

#include "arg_parser.h"

// Constructor that automatically parses the arguments
ArgumentParser::ArgumentParser(int argc, char *argv[]) {
    parseArguments(argc, argv);
}

// Retrieve a value for an option (e.g., "-a" returns "auth_file")
string ArgumentParser::getOption(const string &option) const {
    if (options.find(option) != options.end()) {
        return options.at(option);
    }
    return "";
}

// Check if a flag (e.g., "-T") is present
bool ArgumentParser::hasFlag(const string &flag) const {
    return flags.count(flag) > 0;
}

// Retrieve positional arguments
vector<string> ArgumentParser::getPositionalArgs() const {
    return positionalArgs;
}

// Check if there are unexpected arguments
bool ArgumentParser::hasUnexpectedArgs() const {
    return !unexpectedArgs.empty();
}

// Function to parse command-line arguments
void ArgumentParser::parseArguments(int argc, char *argv[]) {
    const vector<string> validOptions = {"-p", "-a", "-o", "-b", "-c", "-C"};
    const vector<string> validFlags = {"-T", "-n", "-h", "-help"};

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];

        if (arg[0] == '-') {  // If the argument starts with '-', it's an option or a flag
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                // If the next argument doesn't start with '-', it's a value for the current option
                if (find(validOptions.begin(), validOptions.end(), arg) != validOptions.end()) {
                    options[arg] = argv[++i];
                } else {
                    // If the option is not valid, store it as unexpected
                    unexpectedArgs.push_back(arg);
                    ++i;  // Skip the associated value
                }
            } else {
                // Otherwise, it's a flag
                if (find(validFlags.begin(), validFlags.end(), arg) != validFlags.end()) {
                    flags[arg] = true;
                } else {
                    // If the flag is not valid, store it as unexpected
                    unexpectedArgs.push_back(arg);
                }
            }
        } else {
            // If it doesn't start with '-', treat it as a positional argument
            positionalArgs.push_back(arg);
        }
    }
}
