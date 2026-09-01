# Report Update Notes

The supplied report describes a console-based Dijkstra system using an adjacency matrix, with O(V²) analysis and a static dataset.

For the upgraded version, replace the corresponding implementation/result sections with the following concepts:

## Updated methodology

The system represents the campus as a weighted undirected graph using an adjacency-list structure. Each edge stores a measured/estimated walking distance plus route metadata such as walking speed, accessibility, lighting, risk, blocked state, verification state, and source.

Dijkstra's algorithm remains the primary shortest-path algorithm. A binary min-heap is used as the priority queue to improve minimum-node extraction.

## Updated features

- Shortest-distance routing
- Fastest-time routing
- Accessibility-aware routing
- Safer walking preference
- Alternative routes
- Dynamic road closures
- Searchable location directory
- Nearest facility search
- Emergency route assistance
- Turn-by-turn path output
- ETA and distance reporting
- Data accuracy audit
- Persistent field-measurement overrides

## Updated performance discussion

For a sparse campus graph, adjacency-list storage is substantially more appropriate than a dense adjacency matrix. With a binary heap, Dijkstra runs in O((V+E) log V), with O(V+E) graph storage.

## Updated limitation statement

The seed graph is inherited from the supplied course project and is not an authoritative GIS survey. Therefore the routing engine should be described as algorithmically accurate for the stored graph, while physical map accuracy remains dependent on field verification.

## Updated future work

- Field survey / authoritative campus GIS data
- Finer-grained walking junctions
- GPS map matching
- Live construction and closure feeds
- Real-time opening-hours/access restrictions
- Mobile/web UI connected to the C routing engine
- Multilingual voice guidance
- Full K-shortest-path / Yen algorithm
- A* search using verified node coordinates
