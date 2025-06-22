/**************************
 * IMAP4rev1 client 
 * Author: Marek Joukl
 * Date: 15.11. 2024 
 * Login: xjoukl00
**************************/

#include "imap.h"

bool connectToServer(int &sockfd, const string &server, int port) {
    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "Error: Could not create socket" << endl;
        return 1;
    }

    // Prepare hints for getaddrinfo
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;         // Use IPv4
    hints.ai_socktype = SOCK_STREAM;   // Use TCP

    string portStr = to_string(port);

    // Get the server address info
    if (getaddrinfo(server.c_str(), portStr.c_str(), &hints, &res) != 0) {
        cerr << "Není možné ověřit identitu serveru " << server << endl;
        return false;
    }

    // Connect to the server
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        cerr << "Není možné se připojit k serveru " << server << " na portu " << port << endl;
        freeaddrinfo(res);
        return false;
    }
    
    freeaddrinfo(res);
    return true;
}

bool authenticate(int sockfd, const string &username, const string &password) {
    // Buffer to receive the server response
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    // Read and check the initial server greeting
    int bytesReceived = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived < 0) {
        cerr << "Error: Unable to read server greeting." << endl;
        return false;
    }

    // Confirm that the server sent an "OK" in the greeting
    if (strstr(buffer, "OK") == NULL) {
        cerr << "Error: Server does not support IMAP or is not ready." << endl;
        return false;
    }

    // Send the LOGIN command
    string tag = generateTag();
    string loginCommand = tag + " LOGIN" + username + password + "\r\n";

    if (send(sockfd, loginCommand.c_str(), loginCommand.length(), 0) < 0) {
        cerr << "Error: Failed to send authentication command." << endl;
        return false;
    }

    // Receive the server response for authentication
    memset(buffer, 0, sizeof(buffer));
    bytesReceived = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived < 0) {
        cerr << "Error: Unable to receive server response after LOGIN." << endl;
        return false;
    }


    // Check for the "OK" response
    if (strstr(buffer, "OK") != NULL) {
        return true;
    } else {
        cerr << "Authentification of user" << username << " was NOT succesful." << endl;
        return false;
    }
}

bool logout(int sockfd) {
    string tag = generateTag();
    string logoutCommand = tag + " LOGOUT\r\n";

    if (send(sockfd, logoutCommand.c_str(), logoutCommand.length(), 0) < 0) {
        cerr << "Error: Failed to send LOGOUT command to the server." << endl;
        return false;
    }

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    // Receive the server's response to LOGOUT
    int bytesReceived = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived < 0) {
        cerr << "Error: Could not receive server response for LOGOUT." << endl;
        return false;
    }

    // Check if the server responded with "BYE"
    if (strstr(buffer, "BYE") != NULL) {
        return true;
    } else {
        return false;
    }
}

int selectMailbox(int sockfd, const string &mailbox) {
    string tag = generateTag();
    string selectCommand = tag + " SELECT " + mailbox + "\r\n";

    // Send the SELECT command to choose the mailbox
    int bytesSent = send(sockfd, selectCommand.c_str(), selectCommand.length(), 0);
    if (bytesSent < 0) {
        cerr << "Error: Failed to send SELECT command." << endl;
        return -1;
    }

    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));

    // Read the response from the server
    int bytesReceived = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived < 0) {
        cerr << "Error: Could not receive SELECT response from server." << endl;
        return -1;
    }

    if (bytesReceived < 0) {
        cerr << "Error: Could not receive SELECT response from server." << endl;
        return -1;
    }

    string response;
    response.assign(buffer, bytesReceived);

    regex uidvalidity_regex(R"(UIDVALIDITY (\d+))");
    smatch match;

    if (regex_search(response, match, uidvalidity_regex)) {
        return stoi(match.str(1));
    }
    if (response.find("NO Mailbox not found") != string::npos) {
        cerr << "Unable to select mailbox: " << mailbox << endl;
        return -1;
    }

    cerr << "Error: UIDVALIDITY not found in the SELECT response for mailbox '" << mailbox << "'." << endl;
    return -1;
}

vector<int> searchMessages(int sockfd, bool newMessagesOnly) {
    string tag = generateTag();
    string searchCommand = tag + " UID SEARCH " + (newMessagesOnly ? "UNSEEN" : "ALL") + "\r\n";

    // Send the UID SEARCH command to the server
    if (send(sockfd, searchCommand.c_str(), searchCommand.length(), 0) < 0) {
        cerr << "Error: Could not send SEARCH command." << endl;
        return {};
    }

    // Buffer to receive the search results
    char buffer[8192];
    memset(buffer, 0, sizeof(buffer));

    // Read the response from the server
    int bytesReceived = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived < 0) {
        cerr << "Error: Could not receive response for SEARCH command." << endl;
        return {};
    }

    string response(buffer);
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

bool fetchAndSaveMessage(int sockfd, int messageUID, const string &outDir, bool headersOnly, string mailbox, string server) {
    string tag = generateTag();

    // Buffer to receive the message
    char buffer[16384];
    memset(buffer, 0, sizeof(buffer));

    string fetchHeaderCommand = tag + " UID FETCH " + to_string(messageUID) + 
                                " BODY[HEADER.FIELDS (DATE FROM TO SUBJECT MESSAGE-ID)]\r\n";

    if (send(sockfd, fetchHeaderCommand.c_str(), fetchHeaderCommand.length(), 0) < 0) {
        cerr << "Error: Failed to send UID FETCH command for headers of message " << messageUID << "." << endl;
        return false;
    }

    // Read the header response from the server
    int bytesReceived = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived < 0) {
        cerr << "Error: Could not receive header response for message " << messageUID << " from server." << endl;
        return false;
    }

    // Save the header response as a string
    string headerResponse;
    headerResponse.assign(buffer, bytesReceived);

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

    if (send(sockfd, fetchBodyCommand.c_str(), fetchBodyCommand.length(), 0) < 0) {
        cerr << "Error: Failed to send UID FETCH command for body of message " << messageUID << "." << endl;
        return false;
    }

    // Read the body response from the server
    bytesReceived = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived < 0) {
        cerr << "Error: Could not receive body response for message " << messageUID << " from server." << endl;
        return false;
    }

    // Save the body response as a string
    string bodyResponse;
    bodyResponse.assign(buffer, bytesReceived);

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
