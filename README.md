# Cache Simulator

This project implements a cache simulator in C that simulates a limited-memory cache module using the Least Recently Used (LRU) replacement policy. The cache simulator is designed to handle different cache configurations, offering flexibility to simulate real-world scenarios involving memory hierarchies.

## Key Features

- Supports Different Cache Configurations:

  - Direct-mapped cache

  - Fully associative cache

  - Set-associative cache

- LRU (Least Recently Used) Replacement Policy: Efficiently manages cache line replacement when a cache miss occurs.
- Customizable Memory Parameters: The simulator accepts various configurations for fast memory size and main memory size.
- Detailed Hit/Miss Statistics: After processing a stream of memory references, the simulator outputs hit and miss statistics, along with a detailed hit rate.

## How the Cache Works
1. Memory Reference Processing: The simulator breaks the address into a tag, set index, and block offset. It then uses the set index to find the corresponding set.
2. Cache Hit/Miss Check: The tag is compared against the cache lines in the set to check if the requested data is present.
- If a cache hit occurs, the data is returned immediately.
- If a cache miss occurs, a block is fetched from the main memory.
3. LRU Policy: On a miss, if all lines in the set are occupied, the LRU policy is applied to evict the least recently used line and replace it with the new data.
4. Statistics: The simulator tracks the number of hits and misses, which can be displayed at the end of the simulation using the stats command.




















