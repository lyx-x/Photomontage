#ifndef MONTAGE_GRAPH_H
#define MONTAGE_GRAPH_H

#include <queue>
#include <opencv2/highgui.hpp>

using namespace cv;
using namespace std;

struct edge_t {
    int from, to, c, f;
    edge_t(int from, int to, int c): from(from), to(to), c(c), f(0) {}
};

class Graph {
public:
    Graph(unsigned long V);

    void add_edge(int from, int to, int c);
    int maxflow(int s, int t);

    vector<int> nodes_from(int source);
    vector<int> nodes_to(int sink);

private:
    vector<vector<int> > g; // adjacent list
    vector<edge_t> edges;
    vector<int> level; // level graph
    vector<int> current; // save node status when run dfs

    bool level_graph(int s, int t);
    int augment(int s, int t, int max_flow);
};

#endif
