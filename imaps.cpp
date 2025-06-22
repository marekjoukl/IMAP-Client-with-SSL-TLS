/**************************
 * IMAP4rev1 client 
 * Author: Marek Joukl
 * Date: 15.11. 2024 
 * Login: xjoukl00
**************************/

#include "imaps.h"

SSL_CTX *initializeSSL(const string &certFile, const string &certDir) {
    SSL_CTX *ctx = nullptr;

    // Initialize OpenSSL libraries
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    // Create a new SSL context with TLS method
    ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        cerr << "Error: Could not create SSL context." << endl;
        return nullptr;
    }

    // Set options to disable insecure protocols and strengthen security
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);

    // Set the maximum depth for the certificate chain
    SSL_CTX_set_verify_depth(ctx, 4);
    
    // Load the certificate file if specified
    if (!certFile.empty()) {
        if (!SSL_CTX_load_verify_locations(ctx, certFile.c_str(), nullptr)) {
            cerr << "Error: Could not load certificate file: " << certFile << endl;
            SSL_CTX_free(ctx);
            return nullptr;
        }
    }
    // Load the certificate directory if specified
    else if (!certDir.empty()) {
        if (!SSL_CTX_load_verify_locations(ctx, nullptr, certDir.c_str())) {
            cerr << "Error: Could not load certificate directory: " << certDir << endl;
            SSL_CTX_free(ctx);
            return nullptr;
        }
    }
    // If neither certFile nor certDir is provided, use default system paths
    else {
        if (!SSL_CTX_set_default_verify_paths(ctx)) {
            cerr << "Error: Could not set default certificate verification paths." << endl;
            SSL_CTX_free(ctx);
            return nullptr;
        }
    }

    return ctx;
}

BIO* connectToServerBIO(SSL_CTX *ctx, const string &server, int port) {
    string serverPort = server + ":" + to_string(port);

    BIO *bio = BIO_new_ssl_connect(ctx);
    if (!bio) {
        cerr << "Error: Could not create BIO object." << endl;
        return nullptr;
    }
    SSL *ssl = nullptr;
    BIO_get_ssl(bio, &ssl);

    if (!ssl) {
        cerr << "Error: Could not retrieve SSL object from BIO." << endl;
        BIO_free_all(bio);
        return nullptr;
    }
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

    // Set the hostname and port
    BIO_set_conn_hostname(bio, serverPort.c_str());
    
    long certVerificationResult = SSL_get_verify_result(ssl);
    if (certVerificationResult != X509_V_OK) {
        cerr << "Warning: Certificate verification failed: " << X509_verify_cert_error_string(certVerificationResult) << endl;
    }

    // Attempt to connect to the server and establish an SSL/TLS session
    if (BIO_do_connect(bio) <= 0) {
        cerr << "Error: Could not connect to server at " << serverPort << "." << endl;
        ERR_print_errors_fp(stderr);
        BIO_free_all(bio);
        return nullptr;
    }

    return bio;
}

bool authenticateBIO(BIO *bio, const string &username, const string &password) {
    // Buffer to receive the server response
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));


    // Read and check the initial server greeting using BIO_read
    int bytesReceived = BIO_read(bio, buffer, sizeof(buffer) - 1);
    if (bytesReceived <= 0) {
        cerr << "Error: Unable to read server greeting." << endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Confirm that the server sent an "OK" in the greeting
    if (strstr(buffer, "OK") == NULL) {
        cerr << "Error: Server does not support IMAP or is not ready." << endl;
        return false;
    }

    // Generate the login command using a unique tag
    string tag = generateTag();
    string loginCommand = tag + " LOGIN " + username + " " + password + "\r\n";

    // Send the LOGIN command using BIO_write
    int bytesSent = BIO_write(bio, loginCommand.c_str(), loginCommand.length());
    if (bytesSent <= 0) {
        cerr << "Error: Failed to send authentication command." << endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Receive the server response for authentication using BIO_read
    memset(buffer, 0, sizeof(buffer));
    bytesReceived = BIO_read(bio, buffer, sizeof(buffer) - 1);
    if (bytesReceived <= 0) {
        cerr << "Error: Unable to receive server response after LOGIN." << endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Check for the "OK" response in the received data
    if (strstr(buffer, "OK LOGIN Authentication succeeded") != NULL) {
        return true;
    } else {
        cerr << "Ověření uživatele" << username << " se nezdařilo." << endl;
        return false;
    }
}

int selectMailboxBIO(BIO *bio, const string &mailbox) {
    string tag = generateTag();
    string selectCommand = tag + " SELECT " + mailbox + "\r\n";

    // Send the SELECT command using `BIO_write` for secure IMAPS connections
    int bytesSent = BIO_write(bio, selectCommand.c_str(), selectCommand.length());
    if (bytesSent <= 0) {
        cerr << "Error: Failed to send SELECT command." << endl;
        ERR_print_errors_fp(stderr);
        return -1;
    }

    string response;
    if (!readIMAPSResponse(bio, response)) {
        cerr << "Error: Could not receive SELECT response from server." << endl;
        ERR_print_errors_fp(stderr);
        return -1;
    }

    // Extract the UIDVALIDITY using regex
    regex uidvalidity_regex(R"(UIDVALIDITY (\d+))");
    smatch match;
    if (regex_search(response, match, uidvalidity_regex)) {
        return stoi(match.str(1));
    }

    // Check if the mailbox is not found
    if (response.find("NO Mailbox not found") != string::npos) {
        cerr << "Nebylo možné zvolit schránku: " << mailbox << endl;
        return -1;
    }

    cerr << "Error: UIDVALIDITY not found in the SELECT response for mailbox '" << mailbox << "'." << endl;
    return -1;
}


vector<int> searchMessagesBIO(BIO *bio, bool newMessagesOnly) {
    string tag = generateTag();
    string searchCommand = tag + " UID SEARCH " + (newMessagesOnly ? "UNSEEN" : "ALL") + "\r\n";

    // Send the UID SEARCH command to the server using BIO_write
    int bytesSent = BIO_write(bio, searchCommand.c_str(), searchCommand.length());
    if (bytesSent <= 0) {
        cerr << "Error: Could not send SEARCH command." << endl;
        ERR_print_errors_fp(stderr);
        return {};
    }

    string response;

    if (!readIMAPSResponse(bio, response)) {
        cerr << "Error: Could not receive response for SEARCH command." << endl;
        ERR_print_errors_fp(stderr);
        return {};
    }

    if (response.find("NO") != string::npos) {
        cerr << "Error: Server returned NO response for SEARCH command." << endl;
        return {};
    }

    vector<int> messageUIDs;
    size_t searchPos = response.find("* SEARCH");
    if (searchPos != string::npos) {
        // Locate the start and end of the UIDs list
        size_t start = searchPos + 8;  // Skip "* SEARCH "
        size_t end = response.find("\r\n", start);

        // Extract the UIDs as a single string
        if (start < end) {
            string uidsStr = response.substr(start, end - start);

            // Split the UIDs string into individual UIDs
            istringstream uidStream(uidsStr);
            int uid;
            while (uidStream >> uid) {
                messageUIDs.push_back(uid);
            }
        }
    } else {
        cerr << "Error: '* SEARCH' not found in the response." << endl;
    }

    return messageUIDs;
}

bool readIMAPSResponse(BIO *bio, string &response) {
    char buffer[4096];
    int bytesRead = 0;
    response.clear();

    while (true) {
        // Read the server response incrementally
        bytesRead = BIO_read(bio, buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0) {
            if (!BIO_should_retry(bio)) {
                return false;
            }
            continue;
        }

        // Null-terminate the received data and append to response string
        buffer[bytesRead] = '\0';
        response += buffer;
        // Check if we've received a complete IMAP response, indicated by the presence of an "OK" or similar status
        if (regex_search(response, regex(R"(\r\n[a-zA-Z0-9]+\s(OK|NO|BAD)\s.*\r\n)"))) {
            break;
        }
    }
    return true;
}

bool fetchAndSaveMessageBIO(BIO *bio, int messageUID, const string &outDir, bool headersOnly, const string &mailbox, const string &server) {
    string tag = generateTag();

    // Send command to fetch headers
    string fetchHeaderCommand = tag + " UID FETCH " + to_string(messageUID) + 
                                " BODY[HEADER.FIELDS (DATE FROM TO SUBJECT MESSAGE-ID)]\r\n";
    if (BIO_write(bio, fetchHeaderCommand.c_str(), fetchHeaderCommand.length()) <= 0) {
        cerr << "Error: Failed to send UID FETCH command for headers of message " << messageUID << "." << endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Read the header response from the server
    string headerResponse;
    if (!readIMAPSResponse(bio, headerResponse)) {
        cerr << "Error: Could not receive header response for message " << messageUID << " from server." << endl;
        return false;
    }

    // If only headers are requested, save and return
    if (headersOnly) {
        ofstream outFile(outDir + "/" + server + "/" + mailbox + "/message_uid_" + to_string(messageUID) + ".eml");
        if (!outFile) {
            cerr << "Error: Could not open file to save message " << messageUID << "." << endl;
            return false;
        }
        outFile << formatToRFC5322(headerResponse, true);
        outFile.close();
        return true;
    }

    // Fetch the body text separately
    tag = generateTag();
    string fetchBodyCommand = tag + " UID FETCH " + to_string(messageUID) + " BODY[1]\r\n";
    if (BIO_write(bio, fetchBodyCommand.c_str(), fetchBodyCommand.length()) <= 0) {
        cerr << "Error: Failed to send UID FETCH command for body of message " << messageUID << "." << endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Read the body response from the server
    string bodyResponse;
    if (!readIMAPSResponse(bio, bodyResponse)) {
        cerr << "Error: Could not receive body response for message " << messageUID << " from server." << endl;
        return false;
    }

    // Save the message to a file in the specified directory
    ofstream outFile(outDir + "/" + server + "/" + mailbox + "/message_uid_" + to_string(messageUID) + ".eml");
    if (!outFile) {
        cerr << "Error: Could not open file to save message " << messageUID << "." << endl;
        return false;
    }

    outFile << "\r\n" + formatToRFC5322(headerResponse, true) + "\r\n" + formatToRFC5322(bodyResponse, false);
    outFile.close();

    return true;
}

bool logoutBIO(BIO *bio) {
    string tag = generateTag();
    string logoutCommand = tag + " LOGOUT\r\n";

    // Use BIO_write to send the LOGOUT command over the secure connection
    int bytesSent = BIO_write(bio, logoutCommand.c_str(), logoutCommand.length());
    if (bytesSent <= 0) {
        cerr << "Error: Failed to send LOGOUT command to the server." << endl;
        ERR_print_errors_fp(stderr);  // Print detailed OpenSSL errors if any
        return false;
    }

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    // Receive the server's response to the LOGOUT command using BIO_read
    int bytesReceived = BIO_read(bio, buffer, sizeof(buffer) - 1);
    if (bytesReceived <= 0) {
        cerr << "Error: Could not receive server response for LOGOUT." << endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Check if the server responded with "BYE"
    if (strstr(buffer, "BYE") != NULL) {
        return true;
    } else {
        return false;
    }
}
