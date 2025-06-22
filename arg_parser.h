/**************************
 * IMAP4rev1 client 
 * Author: Marek Joukl
 * Date: 15.11. 2024 
 * Login: xjoukl00
**************************/

#ifndef ARG_PARSER_H
#define ARG_PARSER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

using namespace std;

class ArgumentParser {
public:
    // Constructor
    ArgumentParser(int argc, char *argv[]);

    // Retrieve a value for an option (e.g., "-a" returns "auth_file")
    string getOption(const string &option) const;

    // Check if a flag (e.g., "-T") is present
    bool hasFlag(const string &flag) const;

    // Check if there are unexpected arguments
    bool hasUnexpectedArgs() const;

    // Retrieve positional arguments
    vector<string> getPositionalArgs() const;

    vector <string> unexpectedArgs;               // Stores unexpected arguments

private:
    unordered_map<string, string> options;        // Stores options with values
    unordered_map<string, bool> flags;            // Stores flags
    vector<string> positionalArgs;                // Stores positional arguments

    // Function to parse command-line arguments
    void parseArguments(int argc, char *argv[]);
};

#endif
