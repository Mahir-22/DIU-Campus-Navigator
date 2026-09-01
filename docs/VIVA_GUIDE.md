# Viva / Presentation Guide

## 30-second project pitch

"DIU Campus Navigator represents the campus as a weighted graph. Locations are vertices and walkable connections are weighted edges. Dijkstra's algorithm finds the minimum-cost route, while a binary heap makes repeated minimum selection efficient. The upgraded system adds practical routing profiles, route alternatives, accessibility constraints, dynamic closures, emergency routing, nearest-facility search, and verified-distance overrides."

## What is the main algorithm?

Dijkstra's shortest-path algorithm.

All route costs used by the program are non-negative, so Dijkstra is appropriate.

## Why not use the old adjacency matrix?

The old implementation stored a `MAX x MAX` matrix. For a campus graph, most node pairs are not directly connected, so the matrix stores many zeros.

The upgraded version uses an adjacency list, which stores only actual connections, then uses a binary min-heap to select the next minimum-cost node efficiently.

## Complexity

With `V` vertices and `E` edges:

- Dijkstra + binary heap: `O((V+E) log V)`
- Graph storage: `O(V+E)`

Alternative-route generation reruns the route engine for selected temporarily banned edges, so its cost is a small multiple of the Dijkstra cost.

## How is "fastest" different from "shortest"?

Shortest uses distance in meters as the edge cost.

Fastest uses estimated walking time:

`time = distance / walking_speed`

so a longer segment can still be preferred if its walking speed is configured higher.

## How does accessibility work?

The accessibility profile rejects an edge if it is marked inaccessible or identified as stairs.

In a real deployment, the dataset should also model ramps, elevators, gates, surface quality, barriers, and opening restrictions.

## How does safer routing work?

The safer profile combines physical distance with a penalty based on the edge risk factor and adds a penalty for unlit links.

Safety metadata must be field-verified before the system is used for operational safety decisions.

## How does road closure work?

The user can toggle an edge between OPEN and CLOSED. Closed edges are removed from the routing graph, so newly calculated routes automatically avoid them.

## How do alternative routes work?

For the current route, the program temporarily bans individual route edges and runs Dijkstra again. Distinct candidates are collected and ranked by the selected routing profile's objective cost.

This is an educational alternative-route strategy rather than a claim of a complete production K-shortest-path implementation.

## What makes the distance "accurate"?

The algorithm is exact for the graph weights stored in the program.

The physical accuracy depends on the measured edge lengths. The repository therefore distinguishes seed estimates from verified field measurements and supports persistent overrides through `campus_overrides.csv`.

## Strongest future upgrade

Replace the seed graph with a field-verified campus GIS graph at the entrance/decision-point level. Then add live map matching/GPS in a separate application layer while keeping this C routing engine as the algorithmic core.
