/* 
 * DIU Campus Navigator
 * Pure C (C11), zero external libraries.
 *
 * Academic project focus:
 *   - Graph modelling
 *   - Dijkstra shortest-path algorithm
 *   - Binary min-heap priority queue
 *   - Alternative-route generation by edge penalisation/banning
 *   - Profile-based routing (shortest / fastest / accessible / safer)
 *   - Dynamic road closures
 *   - Nearest facility / emergency routing
 *   - Search and campus directory
 *   - Distance-data audit and editable measurement overrides
 *
 * DATA HONESTY:
 * The starter distances below come from the supplied student project.
 * They are not claimed to be GPS/survey-grade. Use the Data Accuracy menu
 * and campus_overrides.csv to replace them with field-measured distances.
 *
 * Compile:
 *   gcc -std=c11 -O2 -Wall -Wextra -pedantic src/campus_navigator.c -lm -o campus_navigator
 *
 * Run:
 *   ./campus_navigator
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#define MAX_NODES 100
#define MAX_EDGES 150
#define MAX_ADJ_PER_NODE 20
#define MAX_PATH 100
#define MAX_CANDIDATES 8
#define NAME_LEN 100
#define LINE_LEN 512
#define INF 1e100
#define OVERRIDE_FILE "campus_overrides.csv"

typedef enum {
    PROFILE_SHORTEST = 1,
    PROFILE_FASTEST,
    PROFILE_ACCESSIBLE,
    PROFILE_SAFER
} RouteProfile;

typedef enum {
    ROAD_PATH = 1,
    ROAD_ROAD,
    ROAD_BRIDGE,
    ROAD_FOOTWAY,
    ROAD_STAIRS,
    ROAD_GATE,
    ROAD_UNKNOWN
} RoadType;

typedef struct {
    int id;
    char name[NAME_LEN];
} Node;

typedef struct {
    int id;
    char from[NAME_LEN];
    char to[NAME_LEN];
    double distance_m;
    double walking_speed_kmh;
    RoadType type;
    int accessible;
    int lit;
    double risk;      /* 0 = unverified/unknown, 1 = low concern, 5 = high concern */
    int blocked;
    int verified;
    char source[80];
} Edge;

typedef struct {
    int edge_id;
    int to;
} Arc;

typedef struct {
    Arc arcs[MAX_ADJ_PER_NODE];
    int count;
} Adjacency;

typedef struct {
    int node;
    double priority;
} HeapItem;

typedef struct {
    HeapItem items[MAX_NODES * 4];
    int size;
} MinHeap;

typedef struct {
    int nodes[MAX_PATH];
    int count;
    double objective_cost;
    double distance_m;
    double time_min;
    double confidence;
} Route;

typedef struct {
    int start_node;
    int end_node;
    int profile;
    Route route;
} CandidateRoute;

static Node nodes[MAX_NODES];
static int node_count = 0;
static Edge edges[MAX_EDGES];
static int edge_count = 0;
static Adjacency graph[MAX_NODES];
static double dist[MAX_NODES];
static int prev_node[MAX_NODES];
static int prev_edge[MAX_NODES];

static const char *profile_name(RouteProfile p) {
    switch (p) {
        case PROFILE_SHORTEST: return "Shortest distance";
        case PROFILE_FASTEST: return "Fastest walking time";
        case PROFILE_ACCESSIBLE: return "Wheelchair/accessibility-aware";
        case PROFILE_SAFER: return "Safer walking preference";
        default: return "Unknown";
    }
}

static const char *road_type_name(RoadType t) {
    switch (t) {
        case ROAD_PATH: return "Campus Path";
        case ROAD_ROAD: return "Road";
        case ROAD_BRIDGE: return "Bridge";
        case ROAD_FOOTWAY: return "Footway";
        case ROAD_STAIRS: return "Stairs";
        case ROAD_GATE: return "Gate";
        default: return "Unknown";
    }
}

static const char *yesno(int v) { return v ? "Yes" : "No"; }

static void trim_newline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}

static void read_line(const char *prompt, char *buf, size_t size) {
    for (;;) {
        printf("%s", prompt);
        if (!fgets(buf, (int)size, stdin)) {
            clearerr(stdin);
            buf[0] = '\0';
            return;
        }
        trim_newline(buf);
        if (buf[0] != '\0') return;
        printf("Input cannot be empty. Try again.\n");
    }
}

static int read_int(const char *prompt, int min, int max) {
    char buf[64];
    char *end;
    long v;
    for (;;) {
        read_line(prompt, buf, sizeof(buf));
        v = strtol(buf, &end, 10);
        while (isspace((unsigned char)*end)) end++;
        if (end != buf && *end == '\0' && v >= min && v <= max) return (int)v;
        printf("Enter a number from %d to %d.\n", min, max);
    }
}

static double read_double(const char *prompt, double min, double max) {
    char buf[64];
    char *end;
    double v;
    for (;;) {
        read_line(prompt, buf, sizeof(buf));
        v = strtod(buf, &end);
        while (isspace((unsigned char)*end)) end++;
        if (end != buf && *end == '\0' && v >= min && v <= max) return v;
        printf("Enter a value from %.2f to %.2f.\n", min, max);
    }
}

static int string_contains_ci(const char *haystack, const char *needle) {
    if (!needle[0]) return 1;
    for (; *haystack; ++haystack) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
            ++h; ++n;
        }
        if (!*n) return 1;
    }
    return 0;
}

static int node_index_from_id(int id) {
    if (id < 1 || id > node_count) return -1;
    return id - 1;
}

static int string_equals_ci(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        ++a; ++b;
    }
    return *a == '\0' && *b == '\0';
}

static int find_node_by_name(const char *name) {
    for (int i = 0; i < node_count; i++) {
        if (string_equals_ci(nodes[i].name, name)) return nodes[i].id;
    }
    return -1;
}

static int find_edge_between(int a_id, int b_id, int *edge_id) {
    for (int i = 0; i < edge_count; i++) {
        int a = find_node_by_name(edges[i].from);
        int b = find_node_by_name(edges[i].to);
        if ((a == a_id && b == b_id) || (a == b_id && b == a_id)) {
            if (edge_id) *edge_id = edges[i].id;
            return 1;
        }
    }
    return 0;
}

static void heap_init(MinHeap *h) { h->size = 0; }

static void heap_push(MinHeap *h, int node, double priority) {
    int i;
    if (h->size >= (int)(sizeof(h->items) / sizeof(h->items[0]))) return;
    i = h->size++;
    h->items[i].node = node;
    h->items[i].priority = priority;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h->items[p].priority <= h->items[i].priority) break;
        HeapItem tmp = h->items[p];
        h->items[p] = h->items[i];
        h->items[i] = tmp;
        i = p;
    }
}

static HeapItem heap_pop(MinHeap *h) {
    HeapItem out = {-1, INF};
    if (h->size == 0) return out;
    out = h->items[0];
    h->items[0] = h->items[--h->size];
    int i = 0;
    while (1) {
        int l = i * 2 + 1, r = i * 2 + 2, s = i;
        if (l < h->size && h->items[l].priority < h->items[s].priority) s = l;
        if (r < h->size && h->items[r].priority < h->items[s].priority) s = r;
        if (s == i) break;
        HeapItem tmp = h->items[i];
        h->items[i] = h->items[s];
        h->items[s] = tmp;
        i = s;
    }
    return out;
}

static void add_node(const char *name) {
    if (node_count >= MAX_NODES) return;
    nodes[node_count].id = node_count + 1;
    snprintf(nodes[node_count].name, NAME_LEN, "%s", name);
    node_count++;
}

static int edge_endpoint_id(const char *name) {
    return find_node_by_name(name);
}

static void rebuild_graph(void) {
    for (int i = 0; i < MAX_NODES; i++) graph[i].count = 0;

    for (int i = 0; i < edge_count; i++) {
        if (edges[i].blocked) continue;
        int a = edge_endpoint_id(edges[i].from);
        int b = edge_endpoint_id(edges[i].to);
        if (a < 1 || b < 1) continue;
        int ai = node_index_from_id(a);
        int bi = node_index_from_id(b);
        if (ai < 0 || bi < 0) continue;

        if (graph[ai].count < MAX_ADJ_PER_NODE) {
            graph[ai].arcs[graph[ai].count++] = (Arc){edges[i].id, b};
        }
        if (graph[bi].count < MAX_ADJ_PER_NODE) {
            graph[bi].arcs[graph[bi].count++] = (Arc){edges[i].id, a};
        }
    }
}

static Edge *get_edge(int edge_id) {
    for (int i = 0; i < edge_count; i++) {
        if (edges[i].id == edge_id) return &edges[i];
    }
    return NULL;
}

static double edge_cost(const Edge *e, RouteProfile profile) {
    double distance = e->distance_m;
    double speed = e->walking_speed_kmh > 0.1 ? e->walking_speed_kmh : 4.5;
    double time_min = distance / (speed * 1000.0 / 60.0);

    switch (profile) {
        case PROFILE_FASTEST:
            return time_min;
        case PROFILE_ACCESSIBLE:
            /* Hard-filtered elsewhere for inaccessible/stairs. */
            return distance + (e->type == ROAD_STAIRS ? 1000000.0 : 0.0);
        case PROFILE_SAFER:
            return distance * (1.0 + 0.12 * fmax(e->risk, 0.0))
                 + (e->lit ? 0.0 : 40.0);
        case PROFILE_SHORTEST:
        default:
            return distance;
    }
}

static int edge_allowed(const Edge *e, RouteProfile profile, int banned_edge) {
    if (e->id == banned_edge || e->blocked) return 0;
    if (profile == PROFILE_ACCESSIBLE && (!e->accessible || e->type == ROAD_STAIRS)) return 0;
    return 1;
}

static int dijkstra(int start_id, RouteProfile profile, int banned_edge) {
    if (node_index_from_id(start_id) < 0) return 0;

    for (int i = 0; i < node_count; i++) {
        dist[i] = INF;
        prev_node[i] = -1;
        prev_edge[i] = -1;
    }

    MinHeap heap;
    heap_init(&heap);
    dist[start_id - 1] = 0.0;
    heap_push(&heap, start_id, 0.0);

    while (heap.size > 0) {
        HeapItem cur = heap_pop(&heap);
        int u = cur.node;
        int ui = node_index_from_id(u);
        if (ui < 0 || cur.priority > dist[ui] + 1e-9) continue;

        for (int j = 0; j < graph[ui].count; j++) {
            Arc a = graph[ui].arcs[j];
            Edge *e = get_edge(a.edge_id);
            if (!e || !edge_allowed(e, profile, banned_edge)) continue;

            int v = a.to;
            int vi = node_index_from_id(v);
            if (vi < 0) continue;

            double nd = dist[ui] + edge_cost(e, profile);
            if (nd + 1e-9 < dist[vi]) {
                dist[vi] = nd;
                prev_node[vi] = u;
                prev_edge[vi] = e->id;
                heap_push(&heap, v, nd);
            }
        }
    }
    return 1;
}

static double path_distance_and_time(const int *path, int count, double *time_min, double *confidence) {
    double total = 0.0;
    double t = 0.0;
    double conf_sum = 0.0;
    int conf_count = 0;

    for (int i = 0; i + 1 < count; i++) {
        int eid = -1;
        if (!find_edge_between(path[i], path[i + 1], &eid)) continue;
        Edge *e = get_edge(eid);
        if (!e) continue;
        double speed = e->walking_speed_kmh > 0.1 ? e->walking_speed_kmh : 4.5;
        total += e->distance_m;
        t += e->distance_m / (speed * 1000.0 / 60.0);
        conf_sum += e->verified ? 1.0 : 0.5;
        conf_count++;
    }

    if (time_min) *time_min = t;
    if (confidence) *confidence = conf_count ? conf_sum / conf_count : 0.0;
    return total;
}

static Route make_route(int start_id, int end_id, RouteProfile profile, int banned_edge) {
    Route r;
    memset(&r, 0, sizeof(r));
    r.objective_cost = INF;

    if (!dijkstra(start_id, profile, banned_edge)) return r;
    if (dist[end_id - 1] >= INF / 2) return r;

    int rev[MAX_PATH];
    int n = 0;
    int cur = end_id;
    while (cur != -1 && n < MAX_PATH) {
        rev[n++] = cur;
        if (cur == start_id) break;
        cur = prev_node[cur - 1];
    }
    if (n == 0 || rev[n - 1] != start_id) return r;

    for (int i = 0; i < n; i++) r.nodes[i] = rev[n - 1 - i];
    r.count = n;
    r.objective_cost = dist[end_id - 1];
    r.distance_m = path_distance_and_time(r.nodes, r.count, &r.time_min, &r.confidence);
    return r;
}

static int same_route(const Route *a, const Route *b) {
    if (a->count != b->count) return 0;
    for (int i = 0; i < a->count; i++) if (a->nodes[i] != b->nodes[i]) return 0;
    return 1;
}

static int build_alternatives(int start_id, int end_id, RouteProfile profile, Route out[MAX_CANDIDATES]) {
    int count = 0;
    Route best = make_route(start_id, end_id, profile, -1);
    if (best.count == 0) return 0;
    out[count++] = best;

    /* Generate candidates by banning each edge of the best/current routes.
       This is intentionally simpler than full Yen's K-shortest path algorithm,
       but still demonstrates systematic alternative-path generation. */
    for (int round = 0; round < 2 && count < MAX_CANDIDATES; round++) {
        for (int i = 0; i + 1 < out[round].count && count < MAX_CANDIDATES; i++) {
            int eid = -1;
            if (!find_edge_between(out[round].nodes[i], out[round].nodes[i + 1], &eid)) continue;
            Route cand = make_route(start_id, end_id, profile, eid);
            if (cand.count == 0) continue;

            int duplicate = 0;
            for (int k = 0; k < count; k++) {
                if (same_route(&cand, &out[k])) { duplicate = 1; break; }
            }
            if (!duplicate) out[count++] = cand;
        }
    }

    /* Sort by actual distance, while preserving the best as rank 1 for
       shortest/accessible/safe profiles only. */
    for (int i = 1; i < count; i++) {
        Route key = out[i];
        int j = i - 1;
        while (j >= 0 && out[j].objective_cost > key.objective_cost) {
            out[j + 1] = out[j];
            j--;
        }
        out[j + 1] = key;
    }
    return count;
}

static void print_path(const Route *r, int rank) {
    printf("\nRoute %d\n", rank);
    printf("  ");
    for (int i = 0; i < r->count; i++) {
        printf("%s", nodes[r->nodes[i] - 1].name);
        if (i + 1 < r->count) printf(" -> ");
    }
    printf("\n  Distance : %.1f m\n", r->distance_m);
    printf("  ETA      : %.1f min (walking baseline)\n", r->time_min);
    printf("  Hops     : %d\n", r->count - 1);
    printf("  Data conf: %s\n", r->confidence >= 0.9 ? "High/verified"
                                             : (r->confidence >= 0.7 ? "Medium"
                                                                      : "Seed estimate / verify"));
}

static void print_directions(const Route *r) {
    printf("\nTurn-by-turn guidance:\n");
    for (int i = 0; i + 1 < r->count; i++) {
        int eid = -1;
        find_edge_between(r->nodes[i], r->nodes[i + 1], &eid);
        Edge *e = get_edge(eid);
        if (!e) continue;
        printf("  %d. Walk %.1f m from %s to %s",
               i + 1, e->distance_m,
               nodes[r->nodes[i] - 1].name,
               nodes[r->nodes[i + 1] - 1].name);
        if (e->type == ROAD_STAIRS) printf(" [STAIRS]");
        if (!e->lit) printf(" [UNLIT]");
        if (e->blocked) printf(" [CLOSED]");
        printf(".\n");
    }
    printf("  %d. Arrive at %s.\n", r->count, nodes[r->nodes[r->count - 1] - 1].name);
}

static void print_header(void) {
    printf("\n==============================================================\n");
    printf("                 DIU CAMPUS NAVIGATOR                         \n");
    printf("==============================================================\n");
    printf("Graph engine: adjacency list + binary min-heap + Dijkstra\n");
    printf("Profiles: shortest | fastest | accessible | safer\n");
    printf("Status: dynamic closures | measurement overrides | emergency mode\n");
    printf("==============================================================\n");
}

static void list_locations(void) {
    printf("\nCampus locations (%d):\n", node_count);
    for (int i = 0; i < node_count; i++) {
        printf("  %2d. %s\n", nodes[i].id, nodes[i].name);
    }
}

static void search_locations(void) {
    char q[NAME_LEN];
    read_line("\nSearch keyword: ", q, sizeof(q));
    int found = 0;
    for (int i = 0; i < node_count; i++) {
        if (string_contains_ci(nodes[i].name, q)) {
            printf("  %2d. %s\n", nodes[i].id, nodes[i].name);
            found++;
        }
    }
    if (!found) printf("No matching location found.\n");
}

static void route_planner(void) {
    list_locations();
    int start = read_int("\nStart location ID: ", 1, node_count);
    int end = read_int("Destination location ID: ", 1, node_count);
    if (start == end) {
        printf("Start and destination are the same.\n");
        return;
    }

    printf("\nRouting profile:\n");
    printf("  1. Shortest distance\n");
    printf("  2. Fastest walking time\n");
    printf("  3. Wheelchair/accessibility-aware\n");
    printf("  4. Safer walking preference\n");
    RouteProfile p = (RouteProfile)read_int("Choose profile: ", 1, 4);

    Route routes[MAX_CANDIDATES];
    int count = build_alternatives(start, end, p, routes);
    if (!count) {
        printf("\nNo route found under the selected profile.\n");
        if (p == PROFILE_ACCESSIBLE)
            printf("Try another profile or check accessibility metadata.\n");
        return;
    }

    printf("\nProfile: %s\n", profile_name(p));
    for (int i = 0; i < count && i < 3; i++) print_path(&routes[i], i + 1);

    int chosen = read_int("\nShow turn-by-turn for route (1-3, 0 skip): ", 0, count < 3 ? count : 3);
    if (chosen > 0) print_directions(&routes[chosen - 1]);
}

static void nearest_facility(void) {
    list_locations();
    int start = read_int("\nYour current location ID: ", 1, node_count);

    printf("\nFacility type:\n");
    printf("  1. Medical / emergency point\n");
    printf("  2. Gate\n");
    printf("  3. Food / dining\n");
    printf("  4. Academic building\n");
    int type = read_int("Choose type: ", 1, 4);

    dijkstra(start, PROFILE_SHORTEST, -1);

    double best = INF;
    int best_id = -1;
    for (int i = 0; i < node_count; i++) {
        const char *name = nodes[i].name;
        int match = 0;
        if (type == 1) match = string_contains_ci(name, "medical");
        if (type == 2) match = string_contains_ci(name, "gate");
        if (type == 3) match = string_contains_ci(name, "food") || string_contains_ci(name, "bonomaya");
        if (type == 4) {
            match = string_contains_ci(name, "building") ||
                    string_contains_ci(name, "lab") ||
                    string_contains_ci(name, "study") ||
                    string_contains_ci(name, "center") ||
                    string_contains_ci(name, "ab1") ||
                    string_contains_ci(name, "ab3") ||
                    string_contains_ci(name, "ab4");
        }
        if (match && dist[i] < best) {
            best = dist[i];
            best_id = nodes[i].id;
        }
    }

    if (best_id < 0 || best >= INF / 2) {
        printf("No matching reachable facility found.\n");
        return;
    }

    Route r = make_route(start, best_id, PROFILE_SHORTEST, -1);
    printf("\nNearest match: %s\n", nodes[best_id - 1].name);
    print_path(&r, 1);
    if (type == 1) printf("Emergency hint: call official campus emergency services if needed.\n");
}

static void emergency_mode(void) {
    list_locations();
    int start = read_int("\nEmergency starting location ID: ", 1, node_count);

    dijkstra(start, PROFILE_SHORTEST, -1);

    int medical_id = -1;
    int gate_id = -1;
    double medical_dist = INF, gate_dist = INF;

    for (int i = 0; i < node_count; i++) {
        if (string_contains_ci(nodes[i].name, "medical") && dist[i] < medical_dist) {
            medical_dist = dist[i];
            medical_id = nodes[i].id;
        }
        if (string_contains_ci(nodes[i].name, "gate") && dist[i] < gate_dist) {
            gate_dist = dist[i];
            gate_id = nodes[i].id;
        }
    }

    printf("\n**************** EMERGENCY MODE ****************\n");
    if (medical_id >= 0) {
        Route r = make_route(start, medical_id, PROFILE_SHORTEST, -1);
        printf("\nNearest medical point:\n");
        print_path(&r, 1);
    }
    if (gate_id >= 0) {
        Route r = make_route(start, gate_id, PROFILE_SHORTEST, -1);
        printf("\nNearest campus gate:\n");
        print_path(&r, 2);
    }
    printf("\nDo not rely on this console for medical or security decisions; use official emergency support.\n");
}

static void show_edge_data(int id) {
    Edge *e = get_edge(id);
    if (!e) return;
    printf("\nEdge #%d\n", e->id);
    printf("  %s <-> %s\n", e->from, e->to);
    printf("  Distance       : %.1f m\n", e->distance_m);
    printf("  Walk speed     : %.1f km/h\n", e->walking_speed_kmh);
    printf("  Type           : %s\n", road_type_name(e->type));
    printf("  Accessible     : %s\n", yesno(e->accessible));
    printf("  Lit            : %s\n", yesno(e->lit));
    printf("  Risk factor    : %.1f / 5.0\n", e->risk);
    printf("  Blocked        : %s\n", yesno(e->blocked));
    printf("  Verified       : %s\n", yesno(e->verified));
    printf("  Source         : %s\n", e->source);
}

static void data_audit(void) {
    int verified = 0, blocked = 0, inaccessible = 0;
    double total = 0.0;
    for (int i = 0; i < edge_count; i++) {
        total += edges[i].distance_m;
        verified += edges[i].verified;
        blocked += edges[i].blocked;
        inaccessible += !edges[i].accessible;
    }

    printf("\n================ DATA ACCURACY AUDIT ================\n");
    printf("Nodes                  : %d\n", node_count);
    printf("Edges                  : %d undirected links\n", edge_count);
    printf("Verified edges         : %d / %d\n", verified, edge_count);
    printf("Blocked edges          : %d\n", blocked);
    printf("Inaccessible edges     : %d\n", inaccessible);
    printf("Seed edge-length total : %.1f m (not a perimeter measurement)\n", total);
    printf("\nIMPORTANT: seed distances are from the supplied course project.\n");
    printf("For high accuracy, replace them with measured path lengths and mark verified.\n");
    printf("This system supports overrides through %s.\n", OVERRIDE_FILE);
    printf("=======================================================\n");
}

static void data_editor(void) {
    printf("\nDATA / ROAD CONTROL\n");
    printf("1. View an edge\n");
    printf("2. Toggle road closure\n");
    printf("3. Update measured distance\n");
    printf("4. Update access/safety metadata\n");
    printf("5. Show full accuracy audit\n");
    printf("6. Save overrides now\n");
    printf("0. Back\n");

    int choice = read_int("Choose: ", 0, 6);
    if (choice == 0) return;

    if (choice == 5) {
        data_audit();
        return;
    }
    if (choice == 6) {
        goto save_now;
    }

    int eid = read_int("Edge ID: ", 1, edge_count);
    Edge *e = get_edge(eid);
    if (!e) return;

    if (choice == 1) {
        show_edge_data(eid);
    } else if (choice == 2) {
        e->blocked = !e->blocked;
        printf("Edge #%d is now %s.\n", eid, e->blocked ? "CLOSED" : "OPEN");
        rebuild_graph();
    } else if (choice == 3) {
        e->distance_m = read_double("New measured distance (meters): ", 0.1, 100000.0);
        e->verified = 1;
        snprintf(e->source, sizeof(e->source), "field measurement / manual override");
        printf("Distance updated and marked verified.\n");
    } else if (choice == 4) {
        e->accessible = read_int("Accessible? (1 yes / 0 no): ", 0, 1);
        e->lit = read_int("Lit? (1 yes / 0 no): ", 0, 1);
        e->risk = read_double("Risk factor (0-5): ", 0.0, 5.0);
        e->verified = read_int("Mark as verified? (1 yes / 0 no): ", 0, 1);
        snprintf(e->source, sizeof(e->source), "manual route metadata");
    }

save_now:
    {
        FILE *fp = fopen(OVERRIDE_FILE, "w");
        if (!fp) {
            printf("Could not write %s.\n", OVERRIDE_FILE);
            return;
        }
        fprintf(fp, "edge_id,distance_m,speed_kmh,accessible,lit,risk,blocked,verified\n");
        for (int i = 0; i < edge_count; i++) {
            fprintf(fp, "%d,%.3f,%.3f,%d,%d,%.3f,%d,%d\n",
                    edges[i].id, edges[i].distance_m, edges[i].walking_speed_kmh,
                    edges[i].accessible, edges[i].lit, edges[i].risk,
                    edges[i].blocked, edges[i].verified);
        }
        fclose(fp);
        printf("Saved overrides to %s\n", OVERRIDE_FILE);
    }
}

static void load_overrides(void) {
    FILE *fp = fopen(OVERRIDE_FILE, "r");
    if (!fp) return;

    char line[LINE_LEN];
    fgets(line, sizeof(line), fp); /* header */

    while (fgets(line, sizeof(line), fp)) {
        int id, accessible, lit, blocked, verified;
        double distance, speed, risk;
        if (sscanf(line, "%d,%lf,%lf,%d,%d,%lf,%d,%d",
                   &id, &distance, &speed, &accessible, &lit, &risk, &blocked, &verified) == 8) {
            Edge *e = get_edge(id);
            if (e) {
                e->distance_m = distance;
                e->walking_speed_kmh = speed;
                e->accessible = accessible;
                e->lit = lit;
                e->risk = risk;
                e->blocked = blocked;
                e->verified = verified;
                snprintf(e->source, sizeof(e->source), "campus_overrides.csv");
            }
        }
    }
    fclose(fp);
}

static void seed_data(void) {
    const struct { int id; const char *name; } node_seed[] = {
    {1, "civil dept building"},
    {2, "guitar tola"},
    {3, "yksg 2"},
    {4, "textile building"},
    {5, "engineering field"},
    {6, "volleyball field"},
    {7, "london bridge"},
    {8, "hotashar mur"},
    {9, "rasg 1 (multipurpose)"},
    {10, "airplane"},
    {11, "zoo"},
    {12, "shadhinota sommelon kendro"},
    {13, "rasg 2"},
    {14, "practice ground"},
    {15, "fountain + diu planet"},
    {16, "bike parking"},
    {17, "ab4"},
    {18, "food court"},
    {19, "diu landmark"},
    {20, "swimming pool"},
    {21, "nishat kabir extension"},
    {22, "yksg 3"},
    {23, "central field"},
    {24, "ab1"},
    {25, "mosque"},
    {26, "basketball ground"},
    {27, "admission office"},
    {28, "shahid minar"},
    {29, "golf field"},
    {30, "gate 2"},
    {31, "gate 3"},
    {32, "green garden"},
    {33, "python road"},
    {34, "transport"},
    {35, "ab3"},
    {36, "diu mini lake"},
    {37, "teachers apartment"},
    {38, "studio apartment"},
    {39, "gate 8"},
    {40, "medical center"},
    {41, "diu garden"},
    {42, "rasg 1"},
    {43, "bonomaya 2"},
    {44, "anisul hoque study center"},
    {45, "bonomaya 1"},
    {46, "innovation lab"},
    {47, "gate 7"},
    {48, "yksg 1"},
    {49, "turf"},
    {50, "dihs"},
    {51, "dutch bangla atm booth"},
    {52, "gate 1"},
    };

    const struct {
        int id; const char *from; const char *to; double dist; double speed;
        RoadType type; int accessible; int lit; double risk; int blocked; int verified; const char *source;
    } edge_seed[] = {
    {1, "civil dept building", "guitar tola", 220.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {2, "guitar tola", "yksg 2", 80.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {3, "guitar tola", "textile building", 25.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {4, "guitar tola", "engineering field", 10.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {5, "engineering field", "textile building", 10.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {6, "engineering field", "yksg 2", 20.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {7, "yksg 2", "volleyball field", 20.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {8, "yksg 2", "london bridge", 80.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {9, "volleyball field", "london bridge", 30.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {10, "london bridge", "hotashar mur", 130.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {11, "hotashar mur", "rasg 1 (multipurpose)", 100.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {12, "hotashar mur", "airplane", 90.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {13, "hotashar mur", "zoo", 260.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {14, "rasg 1 (multipurpose)", "shadhinota sommelon kendro", 10.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {15, "shadhinota sommelon kendro", "rasg 2", 120.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {16, "shadhinota sommelon kendro", "practice ground", 60.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {17, "shadhinota sommelon kendro", "fountain + diu planet", 60.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {18, "rasg 2", "fountain + diu planet", 60.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {19, "rasg 2", "bike parking", 50.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {20, "bike parking", "fountain + diu planet", 40.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {21, "bike parking", "practice ground", 40.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {22, "bike parking", "ab4", 180.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {23, "fountain + diu planet", "practice ground", 10.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {24, "practice ground", "food court", 200.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {25, "practice ground", "diu landmark", 20.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {26, "practice ground", "ab4", 40.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {27, "ab4", "swimming pool", 70.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {28, "ab4", "nishat kabir extension", 140.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {29, "ab4", "yksg 3", 250.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {30, "ab4", "central field", 20.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {31, "ab4", "diu landmark", 40.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {32, "diu landmark", "ab1", 60.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {33, "diu landmark", "central field", 10.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {34, "central field", "ab1", 15.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {35, "central field", "mosque", 10.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {36, "central field", "basketball ground", 10.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {37, "central field", "admission office", 10.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {38, "central field", "shahid minar", 10.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {39, "central field", "golf field", 15.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {40, "central field", "swimming pool", 25.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {41, "swimming pool", "golf field", 40.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {42, "golf field", "shahid minar", 40.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {43, "shahid minar", "admission office", 30.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {44, "admission office", "gate 2", 20.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {45, "admission office", "gate 3", 20.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {46, "admission office", "basketball ground", 50.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {47, "basketball ground", "green garden", 30.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {48, "green garden", "mosque", 80.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {49, "green garden", "python road", 60.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {50, "mosque", "ab1", 50.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {51, "mosque", "python road", 50.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {52, "mosque", "transport", 100.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {53, "python road", "ab3", 15.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {54, "ab3", "transport", 40.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {55, "transport", "diu mini lake", 80.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {56, "transport", "teachers apartment", 60.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {57, "teachers apartment", "studio apartment", 40.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {58, "studio apartment", "gate 8", 30.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {59, "airplane", "medical center", 120.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {60, "medical center", "food court", 80.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {61, "medical center", "diu garden", 40.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {62, "medical center", "rasg 1", 120.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {63, "rasg 1", "diu garden", 50.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {64, "rasg 1", "bonomaya 2", 90.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {65, "zoo", "bonomaya 2", 100.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {66, "bonomaya 2", "anisul hoque study center", 90.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {67, "diu garden", "bonomaya 1", 50.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {68, "food court", "bonomaya 1", 40.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {69, "food court", "innovation lab", 100.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {70, "bonomaya 1", "anisul hoque study center", 125.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {71, "bonomaya 1", "innovation lab", 80.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {72, "bonomaya 1", "diu mini lake", 70.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {73, "anisul hoque study center", "diu mini lake", 80.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {74, "anisul hoque study center", "gate 7", 70.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {75, "innovation lab", "ab1", 30.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {76, "innovation lab", "diu mini lake", 40.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {77, "gate 7", "gate 8", 10.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {78, "gate 8", "yksg 1", 140.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {79, "yksg 1", "turf", 80.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {80, "turf", "dihs", 700.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {81, "turf", "dutch bangla atm booth", 200.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {82, "dutch bangla atm booth", "gate 1", 30.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {83, "gate 1", "gate 2", 50.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {84, "gate 2", "gate 3", 30.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    {85, "gate 3", "nishat kabir extension", 90.0, 4.5, ROAD_PATH, 1, 1, 1.0, 0, 0, "project seed dataset"},
    };

    node_count = 0;
    edge_count = 0;

    for (size_t i = 0; i < sizeof(node_seed) / sizeof(node_seed[0]); i++) add_node(node_seed[i].name);

    for (size_t i = 0; i < sizeof(edge_seed) / sizeof(edge_seed[0]); i++) {
        if (edge_count >= MAX_EDGES) break;
        edges[edge_count].id = edge_seed[i].id;
        snprintf(edges[edge_count].from, NAME_LEN, "%s", edge_seed[i].from);
        snprintf(edges[edge_count].to, NAME_LEN, "%s", edge_seed[i].to);
        edges[edge_count].distance_m = edge_seed[i].dist;
        edges[edge_count].walking_speed_kmh = edge_seed[i].speed;
        edges[edge_count].type = edge_seed[i].type;
        edges[edge_count].accessible = edge_seed[i].accessible;
        edges[edge_count].lit = edge_seed[i].lit;
        edges[edge_count].risk = edge_seed[i].risk;
        edges[edge_count].blocked = edge_seed[i].blocked;
        edges[edge_count].verified = edge_seed[i].verified;
        snprintf(edges[edge_count].source, sizeof(edges[edge_count].source), "%s", edge_seed[i].source);
        edge_count++;
    }

    rebuild_graph();
    load_overrides();
    rebuild_graph();
}

static void show_about(void) {
    printf("\nABOUT\n");
    printf("DIU Campus Navigator — advanced Algorithms Lab edition.\n");
    printf("Core algorithm: Dijkstra shortest path with a binary min-heap.\n");
    printf("Complexity: O((V+E) log V) for each Dijkstra run with adjacency lists.\n");
    printf("This edition adds route profiles, alternatives, closures, accessibility,\n");
    printf("nearest facilities, emergency routing, search, and field-measurement overrides.\n");
    printf("\nAccuracy principle: the algorithm is exact for the graph data it receives;\n");
    printf("real-world accuracy depends on the quality of the campus measurements.\n");
}

int main(void) {
    seed_data();
    print_header();

    for (;;) {
        printf("\nMAIN MENU\n");
        printf("  1. Plan a route\n");
        printf("  2. Search campus locations\n");
        printf("  3. List all locations\n");
        printf("  4. Find nearest facility\n");
        printf("  5. Emergency mode\n");
        printf("  6. Road status & data accuracy\n");
        printf("  7. About / algorithm analysis\n");
        printf("  0. Exit\n");

        int choice = read_int("Choose: ", 0, 7);
        switch (choice) {
            case 1: route_planner(); break;
            case 2: search_locations(); break;
            case 3: list_locations(); break;
            case 4: nearest_facility(); break;
            case 5: emergency_mode(); break;
            case 6: data_editor(); break;
            case 7: show_about(); break;
            case 0:
                printf("\nThank you for using DIU Campus Navigator.\n");
                return 0;
        }
    }
}
