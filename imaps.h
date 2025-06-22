/**************************
 * IMAP4rev1 client 
 * Author: Marek Joukl
 * Date: 15.11. 2024 
 * Login: xjoukl00
**************************/

#ifndef IMAPS_H
#define IMAPS_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include "utils.h"

using namespace std;

/**
 * Initializes the OpenSSL library and creates a new SSL context.
 * @param certFile - The path to the certificate file.
 * @param certDir - The path to the certificate directory.
 * @return - A pointer to the SSL context on success, or nullptr on failure.
 */
SSL_CTX *initializeSSL(const string &certFile = "", const string &certDir = "");

/**
 * Establishes a secure connection to the server using BIO and SSL.
 * Receives and prints the server's initial response.
 * @param ctx - The SSL context to be used for the connection.
 * @param server - The server address.
 * @param port - The port number to connect to.
 * @return A pointer to a connected BIO object on success, or nullptr on failure.
 */
BIO* connectToServerBIO(SSL_CTX *ctx, const string &server, int port);

/**
 * Authenticates a user over a secure IMAP connection using BIO.
 * @param bio - Pointer to the active BIO object.
 * @param username - The username for authentication.
 * @param password - The password for authentication.
 * @return - Returns true if authentication is successful, false otherwise.
 */
bool authenticateBIO(BIO *bio, const string &username, const string &password);

/**
 * Selects a mailbox on a secure IMAPS connection using the BIO library.
 * @param bio - The BIO object for the IMAPS connection.
 * @param mailbox - The name of the mailbox to select (e.g., "INBOX").
 * @return - Returns the UIDVALIDITY if successful, -1 on failure.
 */
int selectMailboxBIO(BIO *bio, const string &mailbox);

/**
 * Sends a UID SEARCH command to the server using a secure BIO connection and retrieves message UIDs.
 * @param bio - The BIO object for the IMAPS connection.
 * @param newMessagesOnly - If true, search for unseen messages only, otherwise search for all messages.
 * @return - A vector of message UIDs that match the search criteria.
 */
vector<int> searchMessagesBIO(BIO *bio, bool newMessagesOnly);

/**
 * Fetch and save a message using a secure BIO connection (IMAPS).
 * @param bio - The BIO object for the secure IMAPS connection.
 * @param messageUID - The UID of the message to fetch.
 * @param outDir - The base output directory.
 * @param headersOnly - If true, only fetch and save the headers.
 * @param mailbox - The mailbox name.
 * @param server - The server name.
 * @return - Returns true if successful, false otherwise.
 */
bool fetchAndSaveMessageBIO(BIO *bio, int messageUID, const string &outDir, bool headersOnly, const string &mailbox, const string &server);

/**
 * Logs out the user from the IMAPS server using a secure BIO connection.
 * @param bio - The BIO object for the secure IMAPS connection.
 * @return - Returns true if the server responds with a "BYE" message, false otherwise.
 */
bool logoutBIO(BIO *bio);

/**
 * Reads the server response from the BIO object and stores it in a string.
 * @param bio - The BIO object for the secure IMAPS connection.
 * @param response - The string to store the server response.
 * @return - Returns true if successful, false otherwise.
 */
bool readIMAPSResponse(BIO *bio, string &response);

#endif // IMAPS_H