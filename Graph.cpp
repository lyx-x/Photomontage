#include "Graph.h"

Graph::Graph(unsigned long V): g(V), level(V), current(V) {}

void Graph::add_edge(int from, int to, int c) {
    g[from].push_back(int(edges.size()));
    edges.push_back(edge_t(from, to, c));
    g[to].push_back(int(edges.size()));
    edges.push_back(edge_t(to, from, 0));
}

bool Graph::level_graph(int s, int t) {
    queue<int> q;
    q.push(s);
    fill(level.begin(), level.end(), -1);
    level[s] = 0;
    while (!q.empty()) {
        int cur = q.front();
        q.pop();
        for (int i: g[cur]) {
            edge_t& e = edges[i];
            if (e.c > e.f && level[e.to] == -1) {
                level[e.to] = level[cur] + 1;
                q.push(e.to);
            }
        }
    }
    return level[t] >= 0;
}

int Graph::augment(int s, int t, int max_flow) {
    if (s == t)
        return max_flow;
    // current child of one node
    for (int i = current[s]; i < g[s].size(); i++) {
        current[s] = i;
        edge_t& e = edges[g[s][i]];
        if (level[e.to] == level[e.from] + 1 && e.c > e.f) {
            int flow = augment(e.to, t, min(max_flow, e.c - e.f));
            if (flow > 0) {
                e.f += flow;
                edges[g[s][i] ^ 1].f -= flow; // opposite edge is near by
                return flow;
            }
        }
    }
    return 0;
}

int Graph::maxflow(int s, int t) {
    int ans = 0;
    while (level_graph(s, t)) { // if s and t are connected
        int flow;
        fill(current.begin(), current.end(), 0);
        do {
            flow = augment(s, t, numeric_limits<int>::max());
            ans += flow;
        } while (flow > 0);
    }
    return ans;
}

vector<int> Graph::nodes_from(int source) {

}

vector<int> Graph::nodes_to(int sink){

}