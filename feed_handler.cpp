
this is my code for feed_handler.cpp

#include "feed_handler.h"
#include "config.h"

// ===================== Logger Implementation =====================

void Logger::log(const std::string &message)
{
    std::cout << getCurrentTime() << " " << message << std::endl;
}

void Logger::error(const std::string &message)
{
    std::cerr << getCurrentTime() << " " << message << std::endl;
}

std::string Logger::getCurrentTime()
{
    time_t now = time(0);
    struct tm tstruct;
    char buffer[80];
    tstruct = *localtime(&now);

    // Format: [28-SEP-2024 12:34:56]
    strftime(buffer, sizeof(buffer), "[%d-%b-%Y %H:%M:%S]", &tstruct);
    return std::string(buffer);
}

// ===================== Packet Implementation =====================

Packet Packet::parse(const char *buffer)
{
    Packet pkt;

    // Extract Symbol (4 bytes)
    pkt.symbol = std::string(buffer, buffer + 4);

    // Extract Buy/Sell Indicator (1 byte)
    pkt.buySellIndicator = buffer[4];

    // Extract Quantity (4 bytes, Big Endian)
    uint32_t quantity_net;
    std::memcpy(&quantity_net, buffer + 5, 4);
    pkt.quantity = ntohl(quantity_net);

    // Extract Price (4 bytes, Big Endian)
    uint32_t price_net;
    std::memcpy(&price_net, buffer + 9, 4);
    pkt.price = ntohl(price_net);

    // Extract Packet Sequence (4 bytes, Big Endian)
    uint32_t seq_net;
    std::memcpy(&seq_net, buffer + 13, 4);
    pkt.packetSequence = ntohl(seq_net);

    return pkt;
}

std::string Packet::toJSON() const
{
    std::ostringstream oss;
    oss << "{"
        << "\"symbol\":\"" << symbol << "\","
        << "\"buySellIndicator\":\"" << buySellIndicator << "\","
        << "\"quantity\":" << quantity << ","
        << "\"price\":" << price << ","
        << "\"packetSequence\":" << packetSequence
        << "}";
    return oss.str();
}

// ===================== FeedHandler Implementation =====================

FeedHandler::FeedHandler(const std::string &serverIP, int port)
    : serverIP_(serverIP), port_(port), sock_(-1) {}

FeedHandler::~FeedHandler()
{
    disconnect();
}

void FeedHandler::connectToServer()
{

    sock_ = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket
    if (sock_ < 0)
    {
        throw std::runtime_error("Socket Creation Failed.");
    }

    struct sockaddr_in serverAddr; // Server address structure
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);

    if (inet_pton(AF_INET, serverIP_.c_str(), &serverAddr.sin_addr) <= 0) // Convert IP address to binary form
    {
        throw std::runtime_error("Invalid IP Address.");
    }

    if (connect(sock_, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) // Connect to the ABX EXCHANGE SERVER
    {
        throw std::runtime_error("Connection to ABX EXCHANGE SERVER Failed.");
    }

    Logger::log("Connected to the ABX EXCHANGE SERVER.");
}

void FeedHandler::disconnect()
{
    if (sock_ != -1)
    {
        close(sock_);
        sock_ = -1;
        Logger::log("Disconnected from the ABX EXCHANGE SERVER");
    }
}

void FeedHandler::setSocketTimeout(int timeoutSeconds)
{
    struct timeval timeout;
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;
    if (setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) < 0)
    {
        throw std::runtime_error("Failed to set socket timeout.");
    }
}

void FeedHandler::sendRequest(const uint8_t *request, size_t length)
{
    ssize_t sentBytes = send(sock_, request, length, 0);
    if (sentBytes < 0)
    {
        throw std::runtime_error("Failed to send request to ABX EXCHANGE SERVER.");
    }
}

void FeedHandler::receiveData()
{
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    while ((bytesReceived = recv(sock_, buffer, BUFFER_SIZE, 0)) > 0)
    {
        parseResponse(buffer, bytesReceived);
    }

    if (bytesReceived == 0)
    {
        Logger::log("ABX EXCHANGE SERVER closed the connection.");
    }
    else if (bytesReceived < 0)
    {
        Logger::error("Receive failed or timed out.");
        throw std::runtime_error("Socket receive FAILED or Timed Out.");
    }
}

void FeedHandler::parseResponse(char *buffer, int bytesReceived)
{
    for (int i = 0; i + PACKET_SIZE <= bytesReceived; i += PACKET_SIZE)
    {
        Packet pkt = Packet::parse(buffer + i);
        receivedSequences_.insert(pkt.packetSequence);
        receivedPackets_.push_back(pkt); // Packet Storage

        // Log the packet details
        std::ostringstream oss;
        oss << "Symbol: " << pkt.symbol
            << ", Buy/Sell: " << pkt.buySellIndicator
            << ", Quantity: " << pkt.quantity
            << ", Price: " << pkt.price
            << ", Packet Seq: " << pkt.packetSequence;
        Logger::log(oss.str());
    }
}

void FeedHandler::requestAllPackets()
{
    uint8_t request[1] = {0x1}; // Call type 1 for "Stream All Packets"
    int retryCount = 0;

    bool responseReceived = false;

    while (!responseReceived && retryCount < MAX_RETRIES)
    {
        try
        {
            sendRequest(request, sizeof(request)); // Send request to the ABX EXCHANGE SERVER
            setSocketTimeout(TIMEOUT_INTERVAL);    // Set socket timeout
            receiveData();                         // Receive and parse response
            responseReceived = true;               // If response is successfully received
        }
        catch (const std::exception &e)
        {
            Logger::error(std::string("No response from ABX EXCHANGE SERVER, retrying request, ERROR: ") + e.what());
            retryCount++;

            // Wait before retrying
            std::this_thread::sleep_for(std::chrono::seconds(RETRY_DELAY));

            if (retryCount >= MAX_RETRIES)
            {
                Logger::error("Max Retries Reached. Stopping further attempts.");
                throw std::runtime_error("Max Retries reached, unable to get a response from the ABX EXCHANGE SERVER.");
            }
        }
    }
}

void FeedHandler::requestPacketBySequence(int sequenceNumber)
{
    uint8_t request[2] = {0x2, static_cast<uint8_t>(sequenceNumber)}; // Call Type 2 + Sequence Number
    bool responseReceived = false;

    while (!responseReceived)
    {
        try
        {

            sendRequest(request, sizeof(request)); // Send request to the ABX EXCHANGE SERVER
            setSocketTimeout(TIMEOUT_INTERVAL);    // Set socket timeout

            // Receive and parse response
            char buffer[BUFFER_SIZE];
            int bytesReceived = recv(sock_, buffer, BUFFER_SIZE, 0);
            if (bytesReceived > 0)
            {
                Packet pkt = Packet::parse(buffer);
                receivedSequences_.insert(pkt.packetSequence);
                receivedPackets_.push_back(pkt); // Packet Storage

                std::ostringstream oss; // Log the packet details
                oss << "Symbol: " << pkt.symbol
                    << ", Buy/Sell: " << pkt.buySellIndicator
                    << ", Quantity: " << pkt.quantity
                    << ", Price: " << pkt.price
                    << ", Packet Seq: " << pkt.packetSequence;
                Logger::log(oss.str());

                responseReceived = true;
            }
            else
            {
                throw std::runtime_error("No DATA received");
            }
        }
        catch (const std::exception &e)
        {
            Logger::error(std::string("NO Response from ABX EXCHANGE SERVER, Retrying REQUEST for SEQUENCE: ") + std::to_string(sequenceNumber) +
                          ". ERROR: " + e.what());
        }
    }
}

void FeedHandler::handleMissingSequences()
{
    if (receivedSequences_.empty())
    {
        Logger::log("NO Packets received to check for missing sequences");
        return;
    }

    int minSeq = std::min(*receivedSequences_.begin(), 1);
    int maxSeq = *receivedSequences_.rbegin();

    for (int seq = minSeq; seq <= maxSeq; ++seq)
    {
        if (receivedSequences_.find(seq) == receivedSequences_.end())
        {
            std::ostringstream oss;
            oss << "Missing Packet with SEQUENCE: " << seq << ", Re-requesting Packet...";
            Logger::log(oss.str());
            requestPacketBySequence(seq);
        }
    }
}

void FeedHandler::writePacketsToJSON(const std::string &filename)
{
    std::sort(receivedPackets_.begin(), receivedPackets_.end());

    std::ofstream jsonFile(filename);
    if (!jsonFile.is_open())
    {
        Logger::error("Failed to open JSON file for dumping MarketData");
        return;
    }

    jsonFile << "[\n";
    for (size_t i = 0; i < receivedPackets_.size(); ++i)
    {
        jsonFile << receivedPackets_[i].toJSON();
        if (i != receivedPackets_.size() - 1)
        {
            jsonFile << ",";
        }
        jsonFile << "\n";
    }
    jsonFile << "]";

    jsonFile.close();
    Logger::log("MD Packets dumped to JSON file: " + filename);
}

// ===================== Main Function =====================

int main()
{

    try
    {
        FeedHandler client(SERVER_IP, SERVER_PORT);

        client.connectToServer(); // Connect to the ABX EXCHANGE SERVER

        client.requestAllPackets(); // Request ALL packets (Call Type 1)

        client.disconnect(); // Disconnect after receiving ALL packets

        client.connectToServer(); // Re-Connect to request missing packets (Call Type 2)

        client.handleMissingSequences(); // Handle MISSING sequences

        client.disconnect(); // Final disconnect

        client.writePacketsToJSON(MARKET_DATA_JSON); // Write all the received packets to a JSON file

        Logger::log("MarketData Feed Handler operation completed successfully.");
    }
    catch (const std::exception &e)
    {
        Logger::error(std::string("ERROR occurred: ") + e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}