#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <cmath>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <functional>

struct Order {
    uint64_t order_id;
    char side;
    double price;
    uint64_t current_size;
};

struct Level {
    double price;
    uint64_t total_size;
    uint32_t order_count;
};

struct Pending {
    bool has_T = false;
    bool has_F = false;
};

using Book = std::map<double, Level, std::function<bool(double, double)>>;
using OrderMap = std::unordered_map<uint64_t, Order>;
using PendingMap = std::unordered_map<uint64_t, Pending>;

class OrderBook {
public:
    OrderBook()
        : bids([](double a, double b) { return a > b; })
        , asks([](double a, double b) { return a < b; }) {}

    void clear() {
        bids.clear();
        asks.clear();
        orders.clear();
        pending_orders.clear();
    }

    void add_order(uint64_t order_id, char side, double price, uint64_t size) {
        Order order{order_id, side, price, size};
        orders[order_id] = order;
        update_book(side, price, size, 1, true);
    }

    void cancel_order(uint64_t order_id, uint64_t remaining_size) {
        auto it = orders.find(order_id);
        if (it == orders.end()) return;

        Order& order = it->second;
        uint64_t current_size = order.current_size;
        if (remaining_size > current_size) return;

        uint64_t amount_to_cancel = current_size - remaining_size;
        int count_change = (remaining_size == 0) ? -1 : 0;
        update_book(order.side, order.price, amount_to_cancel, count_change, false);

        if (remaining_size == 0) {
            orders.erase(it);
        } else {
            order.current_size = remaining_size;
        }
    }

    void record_pending(uint64_t order_id, char action) {
        Pending& pending = pending_orders[order_id];
        if (action == 'T') pending.has_T = true;
        else if (action == 'F') pending.has_F = true;
    }

    char check_pending(uint64_t order_id) {
        auto it = pending_orders.find(order_id);
        if (it == pending_orders.end()) return 'C';
        const Pending& pending = it->second;
        pending_orders.erase(it);
        return (pending.has_T && pending.has_F) ? 'T' : 'C';
    }

    const Book& get_bids() const { return bids; }
    const Book& get_asks() const { return asks; }

private:
    void update_book(char side, double price, uint64_t size_delta, int count_delta, bool is_add) {
        Book& book = (side == 'B') ? bids : asks;
        auto it = book.find(price);
        if (it == book.end()) {
            if (is_add) {
                book[price] = Level{price, size_delta, static_cast<uint32_t>(count_delta)};
            }
        } else {
            Level& level = it->second;
            level.total_size += (is_add) ? size_delta : -static_cast<int64_t>(size_delta);
            level.order_count += count_delta;
            if (level.total_size == 0) {
                book.erase(it);
            }
        }
    }

    Book bids;
    Book asks;
    OrderMap orders;
    PendingMap pending_orders;
};

std::vector<std::string> tokenize(const std::string& line, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream iss(line);
    while (std::getline(iss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

int find_depth(const Book& book, double price) {
    int depth = 0;
    for (const auto& entry : book) {
        if (std::fabs(entry.first - price) < 1e-9) {
            return depth;
        }
        depth++;
    }
    return -1;
}

void write_mbp_row(std::ofstream& out, const std::vector<std::string>& event, char action,
                  char side, int depth, double price, uint64_t size, const OrderBook& book) {
    out << event[0] << ","; // ts_recv
    out << event[1] << ","; // ts_event
    out << "10,";           // rtype
    out << event[3] << ","; // publisher_id
    out << event[4] << ","; // instrument_id
    out << action << ",";   // action
    out << side << ",";     // side
    out << depth << ",";    // depth
    out << price << ",";    // price
    out << size << ",";     // size
    out << event[9] << ","; // flags
    out << event[10] << ","; // ts_in_delta
    out << event[11] << ","; // sequence

    auto write_levels = [&](const Book& side_book, int levels) {
        auto it = side_book.begin();
        for (int i = 0; i < levels; ++i) {
            if (it != side_book.end()) {
                const Level& level = it->second;
                out << level.price << "," << level.total_size << ",1,";
                ++it;
            } else {
                out << ",0,0,";
            }
        }
    };

    write_levels(book.get_bids(), 10);
    write_levels(book.get_asks(), 10);

    out << event[12]; // symbol
    out << "\n";
}

void process_mbo(const std::string& input_file, const std::string& output_file) {
    std::ifstream in(input_file);
    std::ofstream out(output_file);
    if (!in.is_open() || !out.is_open()) return;

    OrderBook order_book;
    std::string line;
    bool first_row = true;

    // Write header
    out << "ts_recv,ts_event,rtype,publisher_id,instrument_id,action,side,depth,price,size,flags,ts_in_delta,sequence,";
    for (int i = 0; i < 10; ++i) out << "bid_px_" << i << ",bid_sz_" << i << ",bid_ct_" << i << ",ask_px_" << i << ",ask_sz_" << i << ",ask_ct_" << i << ",";
    out << "symbol\n";

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> tokens = tokenize(line, ',');

        if (first_row) {
            if (tokens.size() > 5 && tokens[5] == "R") {
                write_mbp_row(out, tokens, 'R', 'N', 0, 0.0, 0, order_book);
                first_row = false;
            }
            continue;
        }

        char action = tokens[5][0];
        char side = tokens[6][0];
        uint64_t order_id = std::stoull(tokens[8]);
        double price = std::stod(tokens[7]);
        uint64_t size = std::stoull(tokens[8]);

        if (action == 'A') {
            order_book.add_order(order_id, side, price, size);
            int depth = find_depth((side == 'B') ? order_book.get_bids() : order_book.get_asks(), price);
            write_mbp_row(out, tokens, 'A', side, depth, price, size, order_book);
        } else if (action == 'C') {
            uint64_t remaining_size = size;
            int depth = find_depth((side == 'B') ? order_book.get_bids() : order_book.get_asks(), price);
            char effective_action = order_book.check_pending(order_id);
            order_book.cancel_order(order_id, remaining_size);
            write_mbp_row(out, tokens, effective_action, side, depth, price, size, order_book);
        } else if (action == 'T' || action == 'F') {
            if (action == 'T' && side == 'N') continue;
            order_book.record_pending(order_id, action);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " mbo.csv\n";
        return 1;
    }
    process_mbo(argv[1], "mbp.csv");
    return 0;
}