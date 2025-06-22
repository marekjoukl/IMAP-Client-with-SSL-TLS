/**************************
 * IMAP4rev1 client 
 * Author: Marek Joukl
 * Date: 15.11. 2024 
 * Login: xjoukl00
**************************/

#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <fstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_set>
#include <sstream>
#include <regex>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

/**
 * Generates a unique IMAP command tag for each command sent to the server.
 * @return - A string representing the command tag in the format "a001", "a002", etc.
 */
string generateTag();

/**
 * Reads the authentication file and extracts the username and password.
 * @param authFile - The path to the authentication file.
 * @return - A pair containing the username and password.
 */
pair<string, string> readAuthFile(const string &authFile);

/**
 * @brief Formats a message to be displayed to the user.
 * 
 * @param mailbox - The name of the mailbox.
 * @param messageCount - The number of messages downloaded.
 * @param newMessagesOnly - A flag indicating whether only new messages were downloaded.
 * @return string 
 */
string formatOutMsg(const string &mailbox, int messageCount, bool newMessagesOnly);

/**
 * Creates a directory structure and stores UIDVALIDITY and UIDs in a state file.
 * @param outDir - Base output directory specified by the user.
 * @param uidvalidity - The UIDVALIDITY value of the selected mailbox.
 * @param mailbox - The mailbox folder to create inside the output directory.
 * @param messageUIDs - A set of UIDs to store the current state (can be empty if not tracking).
 * @return - Returns true if successful, false otherwise.
 */
bool createDir(const string outDir, const int uidvalidity, const string mailbox, vector<int> messageUIDs, string server, bool headersOnly);

/**
 * @brief Prints the help message with usage instructions for the IMAP client.
 */
void printHelp();

/**
 * Updates the state file with the latest UIDVALIDITY and UIDs.
 * @param outDir - Base output directory specified by the user.
 * @param mailbox - The mailbox folder to update inside the output directory.
 * @param uidvalidity - The UIDVALIDITY value of the selected mailbox.
 * @param uids - The updated list of UIDs in the current mailbox.
 */
void updateStateFile(const string &outDir, const string &mailbox, int uidvalidity, const vector<int> &uids, const string server, bool headersOnly);

// Function to format from raw IMAP response to RFC 5322 format
string formatToRFC5322(const string &response, bool isHeader);

/**
 * Checks the stored UIDVALIDITY and UIDs against the current server state to determine which messages should be downloaded.
 * If the state file doesn't exist, it treats the entire mailbox as new and downloads all messages.
 * If the UIDVALIDITY matches, it compares the stored UIDs with the server UIDs to identify any new messages.
 * If the UIDVALIDITY has changed, it treats the mailbox as having a new state and downloads all messages.
 * @param outDir - Base output directory where state information is stored.
 * @param currentUIDValidity - The current UIDVALIDITY value of the selected mailbox.
 * @param mailbox - The mailbox folder to check for state information.
 * @param serverUIDs - The current set of UIDs retrieved from the server for comparison.
 * @param server - The server address used to differentiate between different server states.
 * @return - A vector of UIDs that need to be downloaded. Returns an empty vector if no new messages need to be downloaded.
 */
vector<int> checkValidity(const string &outDir, int currentUIDValidity, const string &mailbox, const vector<int> &serverUIDs, string server, bool headersOnly);

#endif // UTILS_H