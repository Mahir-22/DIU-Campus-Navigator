# Campus Data Accuracy Plan

## 1. What "accurate" means here

There are two separate correctness questions:

1. **Algorithmic correctness:** does the program find the minimum-cost route for the supplied graph?
2. **Map-data correctness:** do the graph's nodes, edges, restrictions, and distances actually match the physical campus?

The program can guarantee the first under its model; it cannot guarantee the second until the dataset is measured and verified.

## 2. Recommended field-survey workflow

For each walkable segment:

- Start at a real decision point: gate, corridor junction, bridge, building entrance, etc.
- End at the next decision point.
- Measure the path people actually walk.
- Record the value in meters with at least one decimal place.
- Record whether the segment is wheelchair accessible.
- Record stairs, barriers, gates, temporary construction, and other access constraints.
- Record lighting status if the safer profile is to be used.
- Mark the segment as verified.

Long edge lengths should be split into smaller segments whenever there are meaningful turns or alternate entrances. This makes turn-by-turn output and dynamic closures much more realistic.

## 3. Recommended validation set

After measurement, create a test matrix containing:

- entrance -> major academic building
- entrance -> medical center
- academic building -> food court
- academic building -> sports facility
- residence -> classroom
- accessibility-sensitive origin/destination pairs
- routes crossing recently closed construction areas

For every test pair, record the field-measured expected route and compare it with program output.

## 4. Why the model is structured this way

Pedestrian routing systems commonly need more than distance alone: crossings, stairs, accessibility, barriers, lighting/safety considerations, and orientation landmarks can affect which path is actually useful.

This project therefore keeps route metadata on each edge rather than hiding everything inside one distance number.

## 5. Versioning

Treat the campus map like software data:

- `graph-data-v1` = seed dataset
- `graph-data-v2` = field verified
- `graph-data-v3` = major physical campus changes

Commit measurement updates separately from algorithm changes whenever possible.
