# ISA Project 2024/2025 - IMAP Client with SSL/TLS

## Author : Marek Joukl (xjoukl00)

## Date : 15.11.2024

## Description:

The aim of the project was to create a programme that can communicate with an IMAP server using the SSL/TLS protocol. The programme is able to authenticate the user, download messages saved of the server, save them in user-specified directory and output the number of downloaded messages. The programme is written in C++ and uses the OpenSSL library.

## Usage:

`make` - compiles the programme

`./imapcl -help` - prints the help message

`./imapcl server [-p port] [-T [-c certfile] [-C certaddr]] [-n] [-h] -a auth_file [-b MAILBOX] -o out_dir` - runs the programme with options:

- `server` - the address of the IMAP server
- `-p port` - the port of the IMAP server (default 143 for unsecured connection)
- `-T` - use SSL/TLS connection (specify the connection port)
- `-c certfile` - the path to the certificate file
- `-C certaddr` - the path to the certificate directory
- `-n` - fetch only new emails
- `-h` - fetch only headers
- `-a auth_file` - the path to the file with the user credentials
- `-b [MAILBOX]` - the name of the mailbox (default INBOX)
- `-o out_dir` - the path to the output directory

## Example:

`./imapcl imap.seznam.cz -T -c cert.pem -a auth.txt -o emails -p 993`

### Prerequisites:

```
sudo apt-get install pkg-config
sudo apt-get install libssl-dev
```

## Files:

- `Makefile` - the makefile for compiling the programme
- `imap.cpp` - functions for handling the unsecured IMAP protocol
- `imap.h` - the header file for the `imap.cpp`
- `imaps.cpp` - functions for handling the secured IMAPS protocol
- `imaps.h` - the header file for the `imaps.cpp`
- `main.cpp` - the main file of the programme
- `utils.cpp` - utility functions for the programme
- `utils.h` - the header file for the `utils.cpp`
- `README.md` - the readme file
- `arg_parser.cpp` - the argument parser for the programme
- `arg_parser.h` - the header file for the `arg_parser.cpp`
