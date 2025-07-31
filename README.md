**Execution**
```
make
./reconstruction mbo.csv
```
**Optimizations**
- Book Updates: O(log N) per event using sorted maps.
- Depth Lookup: O(K) where K is depth position (typically small).
- Combined Events: Only one book update per T→F→C sequence.
  
**Limitations:**
- Depth computation requires linear scan of levels (but stops at target).
- Pending sequence tracking may use extra memory.
  
**Correctness:**
- Matches provided mbp.csv format.
- Handles edge cases (e.g., partial fills, pending sequences).
  
**Performance**
- Processes millions of events/sec on commodity hardware.
- Tested with Valgrind for memory leaks.

