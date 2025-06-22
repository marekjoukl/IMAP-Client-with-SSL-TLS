/**************************
 * IMAP4rev1 client 
 * Author: Marek Joukl
 * Date: 15.11. 2024 
 * Login: xjoukl00
**************************/

#include "utils.h"

int commandCounter = 1;

string generateTag() {
    stringstream ss;
    ss << "a" << setw(3) << setfill('0') << commandCounter++;
    return ss.str();
}

// Function to read the authentication file and extract username and password
pair<string, string> readAuthFile(const string &authFile) {
    ifstream file(authFile);
    string username, password, line;

    if (!file.is_open()) {
        throw runtime_error("Unable to open authentication file: " + authFile);
    }

    // Read the file line by line to extract the username and password
    while (getline(file, line)) {
        if (line.find("username") != string::npos) {
            username = line.substr(line.find('=') + 1);
        } else if (line.find("password") != string::npos) {
            password = line.substr(line.find('=') + 1);
        }
    }

    if (username.empty() || password.empty()) {
        throw runtime_error(" Missing username or password in the authentication file.");
    }
    return {username, password};
}


string formatOutMsg(const string &mailbox, int messageCount, bool newMessagesOnly) {
    string outMsg;

    string messageLabel;
    if (messageCount == 1) {
        messageLabel = newMessagesOnly ? "new message" : "message";
    } else {
        messageLabel = newMessagesOnly ? "new messages" : "messages";
    }
    outMsg = "Downloaded " + to_string(messageCount) + " " + messageLabel + " from mailbox " + mailbox;
    return outMsg;
}

bool createDir(const string outDir, const int uidvalidity, const string mailbox, vector<int> messageUIDs, string server, bool headersOnly) {
    string path = outDir + "/" + server + "/" + mailbox;

    if (!fs::exists(path)) {
        try {
            fs::create_directories(path);
        } catch (const exception &e) {
            cerr << "Error: Could not create directory " << outDir << ". " << e.what() << endl;
            return false;
        }
    }

    // Check if state.txt already exists to merge old UIDs
    unordered_set<int> allUIDs(messageUIDs.begin(), messageUIDs.end());
    string stateFilePath = path + "/state.txt";

    if (fs::exists(stateFilePath)) {
        ifstream stateFile(stateFilePath);
        string line;

        // Read the existing UIDs from the state.txt file
        while (getline(stateFile, line)) {
            if (line.find("UIDs:") != string::npos) {
                istringstream iss(line);
                string key;
                int uid;

                iss >> key;  // Skip the "UIDs:" part
                while (iss >> uid) {
                    allUIDs.insert(uid);  // Add existing UIDs to the set
                }
                break;
            }
        }
        stateFile.close();
    }

    // Open state.txt for writing and save UIDVALIDITY and all UIDs
    ofstream outFile(stateFilePath);
    if (!outFile) {
        cerr << "Error: Could not open file to save UIDVALIDITY." << endl;
        return false;
    }

    outFile << "HeadersOnly: " << boolalpha << headersOnly << endl;

    outFile << "UIDVALIDITY: " << uidvalidity << endl;

    // Write the combined list of UIDs
    outFile << "UIDs: ";
    for (const int uid : allUIDs) {
        outFile << uid << " ";
    }
    outFile << "\n";

    outFile.close();
    return true;
}

void printHelp() {
    cout << "Usage: imapcl server [-p port] [-T [-c certfile] [-C certaddr]] [-n] [-h] -a auth_file [-b MAILBOX] -o out_dir\n\n";

    cout << "Required parameters:\n";
    cout << "  server         The server name (IP address or domain name) of the requested resource.\n";
    cout << "  -a auth_file   File with authentication details (LOGIN command).\n";
    cout << "  -o out_dir     Output directory where the downloaded messages will be saved.\n\n";

    cout << "Optional parameters:\n";
    cout << "  -p port        Specifies the port number on the server. Choose an appropriate default value depending on\n";
    cout << "                 the specification of the -T parameter and the port numbers registered by IANA.\n";
    cout << "  -T             Enables encryption (imaps). If this parameter is not provided, an unencrypted protocol will be used.\n";
    cout << "  -c certfile    File with certificates used to verify the SSL/TLS certificate presented by the server.\n";
    cout << "  -C certaddr    Directory where certificates for verifying the SSL/TLS certificate presented by the server\n";
    cout << "                 are stored. Default value is /etc/ssl/certs.\n";
    cout << "  -n             Only work with new messages (reading).\n";
    cout << "  -h             Download only the headers of messages.\n";
    cout << "  -b MAILBOX     The name of the mailbox to work with on the server. The default value is INBOX.\n\n";
    cout << "  --help         Display this help message.\n\n";


    cout << "Note:\n";
    cout << "  The authentication file (auth_file) must contain the following format:\n";
    cout << "    username = your_username\n";
    cout << "    password = your_password\n\n";
}


void updateStateFile(const string &outDir, const string &mailbox, int uidvalidity, const vector<int> &uids, const string server, bool headersOnly) {
    string stateFilePath = outDir + "/" + server + "/" + mailbox + "/state.txt";

    ofstream stateFile(stateFilePath);
    if (!stateFile) {
        cerr << "Error: Could not open file to update state: " << stateFilePath << "." << endl;
        return;
    }

    stateFile << "HeadersOnly: " << boolalpha << headersOnly << endl;

    stateFile << "UIDVALIDITY: " << uidvalidity << endl;

    // Write the updated UIDs
    stateFile << "UIDs: ";
    for (const int &uid : uids) {
        stateFile << uid << " ";
    }
    stateFile << "\n";

    stateFile.close();
}

string formatToRFC5322(const string &response, bool isHeader) {
    regex first_line_regex(R"(^.*\r?\n)");
    string formatted = regex_replace(response, first_line_regex, "");

    // Removes trailing IMAP commands (OK UID FETCH completed etc.)
    regex imap_command_regex(R"((\r?\n[a-zA-Z0-9]+\sOK\s.*))");
    formatted = regex_replace(formatted, imap_command_regex, "");

    // Remove trailing ")" character
    formatted = regex_replace(formatted, regex(R"(\)\s*$)"), "");

    // Rearrange the headers according to RFC 5322
    if (isHeader) {
        string date, from, to, subject, message_id;
        smatch match;

        if (regex_search(formatted, match, regex(R"(Date: .+?\r?\n)"))) date = match.str();
        if (regex_search(formatted, match, regex(R"(From: .+?\r?\n)"))) from = match.str();
        if (regex_search(formatted, match, regex(R"(To: .+?\r?\n)"))) to = match.str();
        if (regex_search(formatted, match, regex(R"(Subject: .+?\r?\n)"))) subject = match.str();
        if (regex_search(formatted, match, regex(R"(Message-Id: .+?\r?\n)"))) message_id = match.str();

        string formatted_header = date + from + to + subject + message_id;
        return formatted_header;
    } 

    return formatted;
}

vector<int> checkValidity(const string &outDir, int currentUIDValidity, const string &mailbox, const vector<int> &serverUIDs, string server, bool headersOnly) {
    int storedUIDValidity;
    vector<int> storedUIDs;

    string stateFilePath = outDir + "/" + server + "/" + mailbox + "/state.txt";
    ifstream stateFile(stateFilePath);

    if (!stateFile) {
        // state.txt doesn't exist, treat it as a new download
        return serverUIDs;  // Download all messages
    }
    string line;
    while (getline(stateFile, line)) {
        if(line.find("HeadersOnly:") != string::npos){
            if(((line.substr(line.find(":") + 2) == "true") && (headersOnly == false)) || ((line.substr(line.find(":") + 2) == "false") && (headersOnly == true))){
                return serverUIDs;
            }
        }
        if (line.find("UIDVALIDITY:") != string::npos) {
            storedUIDValidity = stoi(line.substr(line.find(":") + 1));  
        } else if (line.find("UIDs:") != string::npos) {
            istringstream uidStream(line.substr(line.find(":") + 1));
            int uid;
            while (uidStream >> uid) {
                storedUIDs.push_back(uid);
            }
        }
    }
    stateFile.close();

    if (storedUIDValidity == currentUIDValidity) {
        // Compare storedUIDs with serverUIDs
        vector<int> newUIDs;
        for (int uid : serverUIDs) {
            if (find(storedUIDs.begin(), storedUIDs.end(), uid) == storedUIDs.end()) {
                newUIDs.push_back(uid);
            }
        }
        return newUIDs;
    } else {
        // UIDVALIDITY changed, download all messages
        return serverUIDs;
    }
}
