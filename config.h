#include <string>

// Server Connection Details
const std::string SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 3000;
const std::string MARKET_DATA_JSON = "output_packets.json";

// Constants for Data Handling
constexpr int BUFFER_SIZE = 1024;
constexpr int TIMEOUT_INTERVAL = 5; // Timeout interval in seconds
constexpr int PACKET_SIZE = 17;     // 4 bytes Symbol + 1 byte Buy/Sell + 4 bytes Quantity + 4 bytes Price + 4 bytes Sequence

// Retry Configurations
const int MAX_RETRIES = 5; // Maximum number of retries (for Call Type 2)
const int RETRY_DELAY = 3; // Delay in seconds before retrying (for Call Type 2)