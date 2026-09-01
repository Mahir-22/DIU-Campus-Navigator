# 🧭 DIU Campus Navigator

### Intelligent Campus Routing Engine — Algorithms Lab Edition

<p align="center">
  <b>Pure C11</b> • <b>Graph Algorithms</b> • <b>Dijkstra</b> • <b>Binary Min-Heap</b> • <b>Campus-Scale Routing</b>
</p>

<p align="center">
  A practical campus navigation engine that combines shortest-path algorithms with accessibility, safety, emergency routing, alternative paths, dynamic closures, and data-verification workflows.
</p>

---

## 🚀 What makes this project different?

This is not just a basic Dijkstra demonstration.

The project treats a campus as a **weighted graph** and turns that graph into a configurable routing engine. The academic core remains Dijkstra's shortest-path algorithm, while the engineering layer adds multiple routing objectives and real-world road metadata.

> **Algorithm accuracy and map accuracy are intentionally separated.** Dijkstra can be exact for the stored graph, but real-world distance accuracy depends on field-verified campus data.

---

## ✨ Feature Matrix

| Navigation | Algorithms | Real-world Intelligence |
|---|---|---|
| 🥇 Shortest-distance route | Dijkstra | ♿ Accessibility-aware routing |
| ⚡ Fastest walking route | Binary min-heap | 🛡️ Safer-route preference |
| 🔀 Alternative routes | Adjacency-list graph | 🚧 Dynamic road closures |
| 🧭 Turn-by-turn route text | Path reconstruction | 🚨 Emergency navigation |
| ⏱️ ETA estimation | Profile-based edge costs | 📏 Distance/data audit |
| 🔎 Campus search | Route ranking | 💾 Persistent measurement overrides |
| 📍 Nearest facility | Input validation | ✅ Verification/confidence reporting |

---

## 🧠 Core Algorithm Architecture

```text
                 CAMPUS LOCATIONS
                        │
                        ▼
              ┌──────────────────┐
              │ Weighted Graph   │
              │ Adjacency List   │
              └────────┬─────────┘
                       │
                       ▼
              ┌──────────────────┐
              │ Routing Profile  │
              │                  │
              │ Shortest         │
              │ Fastest          │
              │ Accessible       │
              │ Safer            │
              └────────┬─────────┘
                       │
                       ▼
              ┌──────────────────┐
              │ Dijkstra +       │
              │ Binary Min-Heap  │
              └────────┬─────────┘
                       │
                       ▼
              ┌──────────────────┐
              │ Path Reconstruction│
              └────────┬─────────┘
                       │
                       ▼
                BEST ROUTE OUTPUT
```

### Complexity

For `V` locations and `E` walkable connections:

- **Dijkstra + binary heap:** `O((V + E) log V)` typical bound
- **Graph storage:** `O(V + E)`
- Alternative routes repeatedly invoke the route engine, so their total cost depends on the number of generated candidates.

---

## 🗺️ Routing Modes

### 🥇 Shortest Distance
Minimizes total walking distance in meters.

### ⚡ Fastest Walking
Minimizes estimated travel time using the configured walking speed of each edge.

### ♿ Accessibility-Aware
Rejects edges marked inaccessible or identified as stairs. For a real deployment, ramps, elevators, surface condition, gates, and barriers should also be field-mapped.

### 🛡️ Safer Walking
Combines distance with configurable risk and lighting penalties. These values must be verified before being treated as operational safety information.

---

## 🚨 Emergency Navigation

Emergency mode is designed for rapid campus assistance workflows:

- Finds the nearest configured medical facility.
- Finds the nearest campus gate.
- Uses the same routing engine as normal navigation.
- Respects currently closed edges.

This is an academic routing feature, **not a replacement for official campus emergency services**.

---

## 🚧 Dynamic Road Control

Edges can be marked **OPEN** or **CLOSED**.

When a connection is closed, future route calculations automatically avoid it. This models temporary construction, maintenance, blocked walkways, or controlled access.

---

## 📏 Data Accuracy Model

Every edge can carry more than a simple distance value:

```text
Distance (meters)
Walking speed (km/h)
Path / road type
Accessibility
Lighting
Risk factor
Open / closed state
Verification status
Source label
```

### Accuracy workflow

1. Split long corridors into actual walking segments and decision points.
2. Measure along the walkable path rather than using straight-line distance.
3. Record stairs, inaccessible links, gates, barriers, and closures.
4. Verify special walking-speed assumptions.
5. Verify lighting/risk metadata on-site.
6. Save measured overrides.
7. Re-run route test cases.
8. Commit the verified dataset separately from the algorithm changes.

> The repository's starter graph is inherited from the supplied course project and should be treated as a **seed dataset**, not as survey-grade GIS data.

---

## 🛠️ Build & Run

### Linux / macOS

```bash
make
./campus_navigator
```

### Windows — MinGW GCC

```bash
gcc -std=c11 -O2 -Wall -Wextra -pedantic src/campus_navigator.c -lm -o campus_navigator.exe
campus_navigator.exe
```

No Python runtime and no external libraries are required.

---

## 🎮 Main Menu

```text
1. Plan a route
2. Search campus locations
3. List all locations
4. Find nearest facility
5. Emergency mode
6. Road status & data accuracy
7. About / algorithm analysis
0. Exit
```

---

## 🧪 Testing

The repository includes a manual test checklist in [`tests/test_cases.txt`](tests/test_cases.txt).

Important scenarios include:

- shortest vs fastest route
- accessibility constraints
- safer routing
- alternative routes
- road closure/reopening
- nearest facility
- emergency navigation
- invalid input handling
- override persistence
- no-route conditions

---

## 📁 Project Structure

```text
DIU-campus-navigator/
├── src/
│   └── campus_navigator.c
├── docs/
│   ├── DATA_ACCURACY.md
│   ├── FEATURE_MATRIX.md
│   ├── REPORT_UPDATE.md
│   └── VIVA_GUIDE.md
├── tests/
│   └── test_cases.txt
├── .gitignore
├── LICENSE
├── Makefile
├── README.md
└── campus_overrides.example.csv
```

---

## 📚 Documentation

- [Data Accuracy Guide](docs/DATA_ACCURACY.md)
- [Feature Matrix](docs/FEATURE_MATRIX.md)
- [Report Update Notes](docs/REPORT_UPDATE.md)
- [Viva Guide](docs/VIVA_GUIDE.md)

---

## 🎓 Academic Value

The project demonstrates practical application of:

- Graph representation
- Weighted graphs
- Dijkstra's shortest-path algorithm
- Priority queues
- Binary heaps
- Path reconstruction
- Complexity analysis
- Greedy optimization
- Input validation
- Data modelling
- State changes in graph routing

It therefore connects the **Algorithms course theory** with a concrete campus-scale problem.

---

## 🧭 Roadmap

- [ ] Field-verified campus walking graph
- [ ] Finer-grained junction/segment modelling
- [ ] Authoritative GIS integration
- [ ] A* routing with verified coordinates
- [ ] Full K-shortest-path implementation (e.g. Yen's algorithm)
- [ ] Live construction/closure feed
- [ ] Opening-hours/access restrictions
- [ ] GPS map matching in a separate UI layer
- [ ] Multilingual/voice guidance
- [ ] Web/mobile front end connected to the C routing engine

---

## ⚠️ Current Limitations

The routing engine is designed as an Algorithms Lab project. Its computational result is only as accurate as the graph data supplied to it. The included seed distances and metadata are not an authoritative campus survey and must be field-verified before real operational use.

---

## 📜 License

Released under the MIT License. See [`LICENSE`](LICENSE).

---

## 👨‍💻 Project

**DIU Campus Navigator — Ultimate Algorithms Lab Edition**

Built as a pure C11 graph-algorithms project with a focus on practical campus navigation, algorithmic correctness, usability, and data quality.
