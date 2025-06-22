/**************************
 * IMAP4rev1 client 
 * Author: Marek Joukl
 * Date: 15.11. 2024 
 * Login: xjoukl00
**************************/

#ifndef IMAP_H
#define IMAP_H

#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <iomanip>
#include <map>
#include "utils.h"

using namespace std;

/**
 * Connects to the specified server using the given port and assigns the socket descriptor.
 * @param sockfd - Reference to the socket file descriptor.
 * @param server - The domain name or IP address of the server.
 * @param port - The port number to connect to.
 * @return - Returns true if the connection is successful, false otherwise.
 */
bool connectToServer(int &sockfd, const string &server, int port);

/**
 * Authenticates the user by sending an IMAP LOGIN command to the server.
 * @param sockfd - The socket file descriptor for the connection.
 * @param username - The username to authenticate with.
 * @param password - The password for the specified username.
 * @return - Returns true if authentication is successful, false otherwise.
 */
bool authenticate(int sockfd, const string &username, const string &password);

/**
 * Logs out the user from the IMAP server by sending a LOGOUT command.
 * @param sockfd - The socket file descriptor for the connection.
 * @return - Returns true if the server responds with a "BYE" message, false otherwise.
 */
bool logout(int sockfd);

/**
 * Selects a specific mailbox on the server using the IMAP SELECT command.
 * @param sockfd - The socket file descriptor for the connection.
 * @param mailbox - The name of the mailbox to select (e.g., "INBOX").
 * @return - Returns UIDVALIDITY number, -1 otherwise.
 */
int selectMailbox(int sockfd, const string &mailbox);

/**
 * Searches for email messages in the currently selected mailbox based on the specified criteria.
 * @param sockfd - The socket file descriptor for the connection.
 * @param newMessagesOnly - If true, only searches for new (unread) messages.
 * @return - A vector of integers representing the IDs of the messages that match the search criteria.
 */
vector<int> searchMessages(int sockfd, bool newMessagesOnly);

/**
 * Fetches and saves a specific email message to a file in the specified output directory.
 * @param sockfd - The socket file descriptor for the connection.
 * @param messageID - The unique ID of the message to fetch.
 * @param outDir - The output directory where the message should be saved.
 * @param headersOnly - If true, only fetches and saves the headers of the message.
 * @return - Returns true if the message is fetched and saved successfully, false otherwise.
 */
bool fetchAndSaveMessage(int sockfd, int messageID, const string &outDir, bool headersOnly, string mailbox, string server);

#endif // IMAP_H