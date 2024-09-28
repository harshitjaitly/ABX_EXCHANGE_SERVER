# ABX Exchange Server

This repository contains the code for the ABX Exchange Server, including the `feed_handler` component. The `feed_handler` is responsible for processing incoming feed data.

## Directory Structure

```
abx_exchange_server/
├── main.js               # ABX Exchange Server executable
├── feed_handler.h        # Header file for feed handler
├── config.h              # Configuration header file
├── feed_handler.cpp      # Implementation of the feed handler
├── feed_handler          # Compiled feed handler executable
└── output_packets.json   # Sample output JSON file
```

## Prerequisites

- A CentOS or similar Linux environment
- A C++ compiler (like `g++`)
- Node.js installed (for running `main.js`)

## Compilation Instructions

To compile the `feed_handler` code, follow these steps:

1. **Open a terminal.**

2. **Navigate to the project directory:**
   ```bash
   cd /path/to/abx_exchange_server
   ```

3. **Compile the `feed_handler` code:**
   ```bash
   g++ -o feed_handler feed_handler.cpp
   ```

   This command compiles `feed_handler.cpp` and generates an executable named `feed_handler`. Ensure that `g++` is installed and available in your PATH.

## Starting the Exchange Server

To start the exchange server application:

1. **Open another terminal.**

2. **Navigate to the project directory (if not already there):**
   ```bash
   cd /path/to/abx_exchange_server
   ```

3. **Run the Node.js server:**
   ```bash
   node main.js
   ```

   This will start the exchange server, and you should see relevant logs indicating that the server is running.


## Running the Feed Handler

Once compiled, you can run the `feed_handler` executable:

1. **Execute the feed handler:**
   ```bash
   ./feed_handler
   ```

2. **Verify that it runs without errors.** The program may produce output or logs depending on its implementation.

## Output

The `feed_handler` will generate output data in the `output_packets.json` file, which can be reviewed for debugging or processing purposes.
