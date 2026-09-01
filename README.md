# DIU Campus Navigator — Ultimate Algorithms Lab Edition

A pure-C campus navigation system built around a weighted graph and Dijkstra's shortest-path algorithm.

## Why this is a major upgrade

The supplied version already models DIU locations as a weighted undirected graph and uses Dijkstra with an adjacency matrix. This version keeps Dijkstra as the academic core but upgrades the engineering around it:

- Adjacency list + binary min-heap priority queue
- Shortest-distance route
- Fastest walking route using edge walking speeds
- Accessibility-aware route (avoids stairs/inaccessible links)
- Safer walking preference
- Up to 3 generated route alternatives
- Dynamic road closures
- Search by location name
- Nearest facility finder
- Emergency mode: nearest medical point + nearest gate
- Turn-by-turn route text
- Distance / ETA / hop / route-confidence reporting
- Data accuracy audit
- Field-measurement override file (`campus_overrides.csv`)
- Manual edge metadata editor
- Robust console input validation
- Pure ISO C11; no Python, no external libraries

## Accuracy philosophy

The algorithm can be exact for the graph data it receives, but it cannot manufacture real-world accuracy from estimated edge lengths.

The starter edge distances in this repository are carried over from the supplied course project. They should therefore be treated as a **seed dataset**, not as survey-grade measurements.

For a genuinely high-accuracy campus deployment, measure each walkable segment on-site (or from an authoritative campus GIS/map source), then enter the measured distance and mark the edge as verified through the Data / Road Control menu. The program can persist those values to `campus_overrides.csv`.

This separation between **routing correctness** and **data correctness** is intentional and is one of the key engineering improvements.

## Current campus-source note

DIU's current official location page lists Daffodil Smart City (DSC), Birulia, Savar, Dhaka-1216, while older DIU material refers to the permanent campus as Ashulia/Dattapara. Because official naming/address information can change over time, the project's campus graph should be treated as a versioned dataset and re-verified before any production deployment.

## Build

### Linux / macOS

```bash
make
./campus_navigator
```

### Windows (MinGW GCC)

```bash
gcc -std=c11 -O2 -Wall -Wextra -pedantic src/campus_navigator.c -lm -o campus_navigator.exe
campus_navigator.exe
```

## Main menu

1. Plan a route
2. Search campus locations
3. List all locations
4. Find nearest facility
5. Emergency mode
6. Road status & data accuracy
7. About / algorithm analysis
0. Exit

## Routing profiles

### Shortest distance
Minimizes the sum of edge lengths in meters.

### Fastest walking time
Minimizes estimated walking time using the speed assigned to each edge.

### Wheelchair/accessibility-aware
Rejects edges marked inaccessible and stair segments. This profile is designed to demonstrate profile-based routing, not to claim that the campus is fully accessibility-mapped.

### Safer walking preference
Uses distance plus penalties for higher risk and unlit segments. Risk/lighting values should be campus-verified before real deployment.

## Data model

Each edge stores:

- distance in meters (`double`)
- walking speed in km/h
- road/path type
- wheelchair/accessibility flag
- lighting flag
- risk factor
- temporary blocked/closed flag
- verification flag
- source label

That makes the system much more realistic than storing only a single integer weight.

## Algorithms

### Dijkstra
The primary route engine remains Dijkstra, which is appropriate because all route costs are non-negative.

### Binary min-heap
The priority queue reduces repeated minimum selection compared with a full `V` scan on every iteration.

### Alternative routes
The project generates route candidates by temporarily banning route edges and rerunning the router, then ranks distinct candidates by the selected profile's objective cost.

This is intentionally presented as an educational alternative-route technique rather than claiming a full production-grade K-shortest-path implementation.

## Complexity

With an adjacency list and binary heap:

- Dijkstra: `O((V + E) log V)` typical bound for non-negative edge weights
- Space: `O(V + E)`

The alternative-route feature repeatedly invokes Dijkstra, so its total cost depends on the number of generated candidates.

## High-accuracy upgrade procedure

1. Split long corridors into actual walking segments and junctions.
2. Measure each segment along the walkable path, not by visual straight-line distance.
3. Mark inaccessible/stair/closed segments explicitly.
4. Record walking speed only where a segment is meaningfully different.
5. Verify lighting/safety metadata on-site.
6. Save the resulting overrides through the built-in editor.
7. Re-run route tests for key origin/destination pairs.
8. Version-control the verified dataset with the code.

## Important project limitation

The included graph is based on the locations and edge lengths from the supplied student code. It is **not** an authoritative DIU GIS dataset. Do not present it as GPS-precise or safety-certified without campus verification.

## Suggested demo scenarios

Use these during viva/demo:

- Civil Dept Building -> Practice Ground
- Gate 1 -> Medical Center
- Any location -> nearest gate
- Accessibility route vs shortest route
- Close a key edge, then calculate again
- Edit a distance, save overrides, restart, and confirm persistence
- Search for `gate`, `field`, `lab`, `bonomaya`, etc.

## Repository structure

```text
diu-campus-navigator/
├── src/
│   └── campus_navigator.c
├── docs/
│   └── DATA_ACCURACY.md
├── .gitignore
├── LICENSE
├── Makefile
├── README.md
└── campus_overrides.csv        # created/updated by the program when used
```

## References

1. Dijkstra, E. W. (1959), shortest-path algorithm.
2. Cormen, Leiserson, Rivest, Stein, *Introduction to Algorithms*.
3. Daffodil International University official location page.
4. Daffodil International University official/archived campus materials.
5. OpenStreetMap routing guidance for pedestrian/accessibility-aware routing.
