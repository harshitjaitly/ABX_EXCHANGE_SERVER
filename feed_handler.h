// feed_handler.h

#ifndef FEED_HANDLER_H
#define FEED_HANDLER_H

#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <sstream>
#include <fstream>

// Logger Class
class Logger
{
public:
    static void log(const std::string &message);
    static void error(const std::string &message);

private:
    static std::string getCurrentTime();
};

// Packet Class
class Packet
{
public:
    std::string symbol;
    char buySellIndicator;
    uint32_t quantity;
    uint32_t price;
    uint32_t packetSequence;

    static Packet parse(const char *buffer);
    std::string toJSON() const;

    // Overloading the less-than operator for sorting based on packetSequence
    bool operator<(const Packet &other) const
    {
        return this->packetSequence < other.packetSequence;
    }
};

// FeedHandler Class
class FeedHandler
{
public:
    FeedHandler(const std::string &serverIP, int port);
    ~FeedHandler();

    void connectToServer();
    void disconnect();

    void requestAllPackets();
    void requestPacketBySequence(int sequenceNumber);
    void handleMissingSequences();

    void writePacketsToJSON(const std::string &filename);

private:
    std::string serverIP_;
    int port_;
    int sock_;
    std::set<int> receivedSequences_;
    std::vector<Packet> receivedPackets_;

    void setSocketTimeout(int timeoutSeconds);
    void sendRequest(const uint8_t *request, size_t length);
    void receiveData();
    void parseResponse(char *buffer, int bytesReceived);
};

#endif // FEED_HANDLER_H