#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

// Define message types
enum MessageType
{
    BEST_BID_ASK = 1,
    TRADE = 2,
    TRADE_CANCELLATION = 3
};

// Define data structures for different message types
struct BestBidAskMessage
{
    string symbol;
    double best_bid;
    double best_ask;
};

struct TradeMessage
{
    string symbol;
    int32_t trade_id;
    int64_t timestamp;
    int32_t quantity;
    double price;
};

struct TradeCancellationMessage
{
    string symbol;
    int32_t trade_id;
};

// Function to read and parse messages from the binary input file
void parseMarketDataFeed(const string &filename)
{
    ifstream file(filename, ios::binary);
    if (!file.is_open())
    {
        cerr << "Failed to open input file: " << filename << endl;
        return;
    }

    unordered_map<string, BestBidAskMessage> best_bid_ask_map;
    unordered_map<string, TradeMessage> latest_trade_map;
    unordered_map<string, int> trade_count_map;
    unordered_map<string, int32_t> total_quantity_map;
    unordered_map<string, double> total_turnover_map;

    while (!file.eof())
    {
        int8_t message_type;
        file.read(reinterpret_cast<char *>(&message_type), sizeof(int8_t));

        switch (static_cast<MessageType>(message_type))
        {
        case BEST_BID_ASK:
        {
            BestBidAskMessage msg;
            char symbol_buf[6];
            file.read(symbol_buf, 6);
            msg.symbol = string(symbol_buf, 6);
            file.read(reinterpret_cast<char *>(&msg.best_bid), sizeof(double));
            file.read(reinterpret_cast<char *>(&msg.best_ask), sizeof(double));
            best_bid_ask_map[msg.symbol] = msg;
            break;
        }
        case TRADE:
        {
            TradeMessage msg;
            char symbol_buf[6];
            file.read(symbol_buf, 6);
            msg.symbol = string(symbol_buf, 6);
            file.read(reinterpret_cast<char *>(&msg.trade_id), sizeof(int32_t));
            file.read(reinterpret_cast<char *>(&msg.timestamp), sizeof(int64_t));
            file.read(reinterpret_cast<char *>(&msg.quantity), sizeof(int32_t));
            file.read(reinterpret_cast<char *>(&msg.price), sizeof(double));

            latest_trade_map[msg.symbol] = msg;
            trade_count_map[msg.symbol]++;
            total_quantity_map[msg.symbol] += msg.quantity;
            total_turnover_map[msg.symbol] += msg.price * msg.quantity;
            break;
        }
        case TRADE_CANCELLATION:
        {
            TradeCancellationMessage msg;
            char symbol_buf[6];
            file.read(symbol_buf, 6);
            msg.symbol = string(symbol_buf, 6);
            file.read(reinterpret_cast<char *>(&msg.trade_id), sizeof(int32_t));

            // Remove the trade entry from the maps if it exists (trade cancellation)
            if (latest_trade_map.find(msg.symbol) != latest_trade_map.end() &&
                latest_trade_map[msg.symbol].trade_id == msg.trade_id)
            {
                trade_count_map[msg.symbol]--;
                total_quantity_map[msg.symbol] -= latest_trade_map[msg.symbol].quantity;
                total_turnover_map[msg.symbol] -= latest_trade_map[msg.symbol].price * latest_trade_map[msg.symbol].quantity;
                latest_trade_map.erase(msg.symbol);
            }
            break;
        }
        default:
            cerr << "Unknown message type: " << static_cast<MessageType>(message_type) << endl;
            return;
        }
    }

    // Print parsed messages
    for (const auto &entry : best_bid_ask_map)
    {
        const BestBidAskMessage &msg = entry.second;
        cout << "Listing: " << msg.symbol << ", Best Bid: " << msg.best_bid << ", Best Ask: " << msg.best_ask << endl;
    }

    for (const auto &entry : latest_trade_map)
    {
        const TradeMessage &msg = entry.second;
        cout << "Listing: " << msg.symbol << ", Last Trade ID: " << msg.trade_id << ", Quantity: " << msg.quantity
             << ", Price: " << msg.price << ", Timestamp: " << msg.timestamp << endl;
    }

    for (const auto &entry : trade_count_map)
    {
        const string &symbol = entry.first;
        int trade_count = entry.second;
        int32_t total_quantity = total_quantity_map[symbol];
        double total_turnover = total_turnover_map[symbol];

        cout << "Listing: " << symbol << ", Number of Trades: " << trade_count << ", Total Quantity: " << total_quantity
             << ", Total Turnover: " << total_turnover << endl;
    }
}

int main()
{
    string filename = "input_feed"; // Replace with the actual filename
    parseMarketDataFeed(filename);
    return 0;
}
