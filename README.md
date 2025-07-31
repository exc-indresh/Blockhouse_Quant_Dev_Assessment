**Execution** 
```
make
./reconstruction mbo.csv
```
# Implementation Details
## Data Structures
### Order Book

- Bids: std::map sorted in descending price order

- Asks: std::map sorted in ascending price order

- Provides O(log N) time complexity for insert/update/delete operations

### Order Tracking

- std::unordered_map for O(1) order lookups by ID

- Tracks order size, price, and side

### Pending Events

- Special handling for T→F→C sequences

- Records pending trade/fill actions before processing cancel

## Event Processing Logic
### Add Order (A):

- Inserts new order into book

- Updates price level aggregates

- Generates immediate MBP snapshot

### Trade (T) and Fill (F):

- Records pending actions without book modification

- Only affects book when followed by Cancel (C)

### Cancel (C):

- Completes pending T→F→C sequence as single trade

- Otherwise processes as regular cancel

- Updates book and generates snapshot

## Performance Optimizations
### Minimal Book Updates:

- Only modifies book on definitive actions (adds/cancels)

- Combines T→F→C sequence into single update

### Efficient Data Structures:

- Sorted maps for fast price level operations

- Hash map for quick order lookups

### Stream Processing:

- Processes input line-by-line with minimal memory usage

- Buffered output writing

# Limitations
### Depth Calculation:

- Currently requires linear scan of price levels

- Could be optimized with additional indexing

### Memory Usage:

- Maintains complete order book in memory

- May need adjustment for extremely large books

### Error Handling:

- Basic input validation

- Assumes well-formed input data

# Future Improvements
### Parallel Processing:

- Could partition book by price ranges

- Process different segments concurrently

### More Efficient Depth Tracking:

- Maintain depth indexes

- Reduce O(K) lookup to O(1)

### Additional Validation:

- More robust input checking

- Sequence number verification

### Performance Profiling:

- Detailed benchmarking

- Hotspot analysis
