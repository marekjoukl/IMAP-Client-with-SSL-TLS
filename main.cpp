/**************************
 * IMAP4rev1 client 
 * Author: Marek Joukl
 * Date: 15.11. 2024 
 * Login: xjoukl00
**************************/

const int IMAP_PORT = 143;

#include "utils.h"
#include "arg_parser.h"
#include "imap.h"
#include "imaps.h"

using namespace std;

int main(int argc, char *argv[]) {
    int sockfd = -1;
    int uidvalidity = -1;
    SSL_CTX *sslCtx = nullptr;
    BIO *bio = nullptr;

    try {
        // Create and initialize the argument parser
        ArgumentParser args(argc, argv);
        if (args.hasUnexpectedArgs()) {
            cerr << "Error: Unexpected arguments provided." << endl;
            return -1;
        }

        if (args.hasFlag("--help")) {
            printHelp();
            return 0;
        }

        // Retrieve values from the argument parser
        vector<string> positionalArgs = args.getPositionalArgs();
        string server = positionalArgs.empty() ? "" : positionalArgs[0];
        int port;
        try {
            port = args.getOption("-p").empty() ? IMAP_PORT : stoi(args.getOption("-p"));
        } catch (const std::invalid_argument &e) {
            cerr << "Error: The specified port is not a valid number." << endl;
            return -1;
        }
        bool useSSL = args.hasFlag("-T");

        string authFile = args.getOption("-a");
        string outDir = args.getOption("-o");
        string mailbox = args.getOption("-b").empty() ? "INBOX" : args.getOption("-b");
        bool newMessagesOnly = args.hasFlag("-n");
        bool headersOnly = args.hasFlag("-h");
        
        string certificateFile = args.getOption("-c").empty() ? "" : args.getOption("-c");
        string certDirectory = args.getOption("-C").empty() ? "/etc/ssl/certs" : args.getOption("-C");

        // Check if required arguments are provided
        if (server.empty() || authFile.empty() || outDir.empty()) {
            cerr << "Usage: ./imapcl server [-p port] [-T] -a auth_file -o out_dir" << endl;
            return -1;
        }

        // Read authentication data from the file
        auto [username, password] = readAuthFile(authFile);
        vector<int> serverUIDs;

        if (useSSL) {
            sslCtx = initializeSSL(certificateFile, certDirectory);
            if (!sslCtx) return -1;

            bio = connectToServerBIO(sslCtx, server, port);
            if (!bio) {
                SSL_CTX_free(sslCtx);
                return -1;
            }
            if(!authenticateBIO(bio, username, password)) {
                BIO_free_all(bio);
                SSL_CTX_free(sslCtx);
                return -1;
            }
            if ((uidvalidity = selectMailboxBIO(bio, mailbox)) == -1) {
                BIO_free_all(bio);
                SSL_CTX_free(sslCtx);
                return -1;
            }
            serverUIDs = searchMessagesBIO(bio, newMessagesOnly);

        } else {
            if (!connectToServer(sockfd, server, port)) {
                close(sockfd);
                return -1;
            }
            // Authenticate using the provided credentials
            if (!authenticate(sockfd, username, password)) {
                close(sockfd);
                return -1;
            }
            // Select the mailbox
            if ((uidvalidity = selectMailbox(sockfd, mailbox)) == -1) {
                close(sockfd);
                return -1;
            }
            // Search for messages in the mailbox
            serverUIDs = searchMessages(sockfd, newMessagesOnly);
        }

        if (serverUIDs.empty()) {
            string outMsg = (newMessagesOnly ? "No new messages found in the mailbox: " : "No messages found in the mailbox: ") + mailbox;
            cout << outMsg << endl;
        } else {
            
            // Check if the directory is valid and if we need to download any new messages
            vector<int> uidsToDownload = checkValidity(outDir, uidvalidity, mailbox, serverUIDs, server, headersOnly);

            if (uidsToDownload.empty()) {
                cout << "Mailbox " << mailbox << " is up to date." << endl;
            } else {
                // Create the directory if it doesn't exist and store the current state
                createDir(outDir, uidvalidity, mailbox, uidsToDownload, server, headersOnly);

                // Fetch and save each message using the UIDs that need to be downloaded
                for (int messageUID : uidsToDownload) {
                    bool fetchSuccess;

                    // Call the appropriate fetch function based on connection type
                    if (useSSL) {
                        fetchSuccess = fetchAndSaveMessageBIO(bio, messageUID, outDir, headersOnly, mailbox, server);
                    } else {
                        fetchSuccess = fetchAndSaveMessage(sockfd, messageUID, outDir, headersOnly, mailbox, server);
                    }

                    // Check if fetching was successful
                    if (!fetchSuccess) {
                        cerr << "Error: Failed to fetch or save message with UID " << messageUID << endl;
                    }
                }
                string outMsg = formatOutMsg(mailbox, uidsToDownload.size(), newMessagesOnly);
                cout << outMsg << endl;
            }
            // Update the state file with the new UIDs after download
            updateStateFile(outDir, mailbox, uidvalidity, serverUIDs, server, headersOnly);
        }

        // Logout and close the connection
       if (useSSL ? !logoutBIO(bio) : !logout(sockfd)) cerr << "Error: Logout failed." << endl;
    } catch (const exception &ex) {
        cerr << "Error: " << ex.what() << endl;
        if (sockfd != -1) close(sockfd);
        if (bio) BIO_free_all(bio);
        if (sslCtx) SSL_CTX_free(sslCtx);
        return -1;
    }
    if (sockfd != -1) close(sockfd);
    if (bio) BIO_free_all(bio);
    if (sslCtx) SSL_CTX_free(sslCtx);
    return 0;
}
