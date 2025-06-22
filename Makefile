# Makefile for imapcl IMAP client

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -g

# pkg-config to get OpenSSL paths
LIBS = $(shell pkg-config --libs openssl)
INCLUDE = $(shell pkg-config --cflags openssl) -I.

TARGET = imapcl

# Source files
SRCS = main.cpp imap.cpp utils.cpp imaps.cpp arg_parser.cpp
HDRS = arg_parser.h imap.h utils.h imaps.h

# Object files
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBS) -o $(TARGET)

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET) -a auth_file -o maildir imap.centrum.sk

pack: clean
	tar --exclude='.vscode' --exclude='.git' --exclude='.gitignore' --exclude='.DS_Store' -cf xjoukl00.tar *

.PHONY: all clean run