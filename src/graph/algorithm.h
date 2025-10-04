#include "data.h"
#include <algorithm>
#include <expected>
#include <print>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace GraphLib::algorithm {

template <isVertex V>
void addOrRemove(Graph<V>& graph, const std::vector<Edge>& edges) {
    for(const auto& ed: edges) {
        if(!graph.delEdge(ed.id)) {
            graph.addEdge(ed);
        }
    }
}

template <isVertex V>
std::vector<int> tarjan(const Graph<V>& graph) {
    const auto vertices = graph.getAllVertices();
    const auto n = vertices.size();
    std::unordered_map<int, int> idToIndex;
    for(int i = 0; i < n; i++) {
        idToIndex[vertices[i].id] = i + 1;
    }

    std::vector<int> d(n + 1, -1);
    std::vector<int> low(n + 1);
    std::vector<int> parent(n + 1, -1);
    std::vector<int> childrenNum(n + 1, 0);
    std::vector<bool> vis(n + 1, false);
    int time = 0;
    std::vector<int> cuts;

    auto dfscv = [&](auto&& self, int index) -> void {
        time++;
        d[index] = low[index] = time;
        vis[index] = true;

        // 获取当前顶点的所有边ID
        auto edgeIdsExp = graph.getEdgeIdsOfVertex(vertices[index - 1].id);
        if(!edgeIdsExp) {
            throw std::runtime_error("Failed to get edge ids of vertex");
        }
        const auto edgeIds = *edgeIdsExp;

        for(int edgeId: edgeIds) {
            const auto& edge = graph.getEdge(edgeId);
            int toIndex = idToIndex[edge.to];

            if(vis[toIndex] == false) {
                parent[toIndex] = index;
                childrenNum[index]++;
                self(self, toIndex);
                low[index] = std::min(low[index], low[toIndex]);

                // 检查是否为切点
                if(parent[index] == -1 && childrenNum[index] > 1) {
                    cuts.push_back(vertices[index - 1].id);
                }
                if(parent[index] != -1 && low[toIndex] >= d[index]) {
                    cuts.push_back(vertices[index - 1].id);
                }
            } else if(toIndex != parent[index]) {
                low[index] = std::min(low[index], d[toIndex]);
            }
        }
    };

    for(int i = 1; i <= n; i++) {
        if(vis[i] == false) {
            dfscv(dfscv, i);
        }
    }

    // 去除重复的切点
    std::sort(cuts.begin(), cuts.end());
    cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());

    return cuts;
}

template <isVertex V>
int distanceWithoutWeight(const Graph<V>& graph, int from, int to) {
    if(from == to) {
        return 0;
    }

    std::unordered_map<int, int> dist;
    std::unordered_set<int> visited;
    std::queue<int> q;

    q.push(from);
    dist[from] = 0;

    while(!q.empty()) {
        int curr = q.front();
        q.pop();

        if(visited.count(curr)) {
            continue;
        }
        visited.insert(curr);

        auto edgeIdsExp = graph.getEdgeIdsOfVertex(curr);
        if(!edgeIdsExp) {
            throw std::runtime_error("Failed to get edge ids of vertex");
        }
        const auto edgeIds = *edgeIdsExp;

        for(int edgeId: edgeIds) {
            const auto& edge = graph.getEdge(edgeId);
            if(!visited.count(edge.to)) {
                dist[edge.to] = dist[curr] + 1;
                if(edge.to == to) {
                    return dist[edge.to];
                }
                q.push(edge.to);
            }
        }
    }
    return -1;
}

template <isVertex V>
std::expected<std::vector<int>, std::string> isBipartite(const Graph<V>& graph) {
    auto vertices = graph.getAllVertices();
    std::unordered_map<int, int> idToIndex;
    for(int i = 0; i < vertices.size(); i++) {
        idToIndex[vertices[i].id] = i;
    }

    std::vector<int> color(vertices.size(), -1);

    auto dfs = [&](auto&& self, int index, int c) -> bool {
        color[index] = c;

        auto edgeIdsExp = graph.getEdgeIdsOfVertex(vertices[index].id);
        if(!edgeIdsExp) {
            throw std::runtime_error("Failed to get edge ids of vertex");
        }
        const auto edgeIds = *edgeIdsExp;

        for(int edgeId: edgeIds) {
            const auto& edge = graph.getEdge(edgeId);
            int toIndex = idToIndex[edge.to];

            if(color[toIndex] == -1) {
                if(!self(self, toIndex, 1 - c)) {
                    return false;
                }
            } else if(color[toIndex] == c) {
                return false;
            }
        }
        return true;
    };

    for(int i = 0; i < vertices.size(); i++) {
        if(color[i] == -1) {
            if(!dfs(dfs, i, 0)) {
                return std::unexpected("The graph is not bipartite");
            }
        }
    }

    // 返回一个分部
    std::vector<int> part;
    for(int i = 0; i < vertices.size(); i++) {
        if(color[i] == 0) {
            part.push_back(vertices[i].id);
        }
    }
    return part;
}

/// You must ensure the graph is bipartite before using this function.
template <isVertex V>
std::unordered_map<int, int> getMaxMatchByHopcraftKarp(Graph<V>& graph,
                                                       const std::vector<int>& onePartIds) {
    std::vector<int> part1 = onePartIds, part2;
    const auto vertices = graph.getAllVertices();
    std::unordered_map<int, int> idToIndex;

    for(int i = 0; i < vertices.size(); i++) {
        idToIndex[vertices[i].id] = i;
    }

    // 构建第二个分部
    for(const auto& v: vertices) {
        if(std::find(part1.begin(), part1.end(), v.id) == part1.end()) {
            part2.push_back(v.id);
        }
    }

    const int n = vertices.size();
    const int INF = 1e9;
    std::vector<int> xMatch(n, -1);
    std::vector<int> yMatch(n, -1);
    std::vector<int> dx(n);
    std::vector<int> dy(n);
    std::vector<bool> vis(n);
    std::queue<int> q;
    int dis = INF;

    auto hkBFS = [&]() -> bool {
        q = std::queue<int>();
        dis = INF;
        std::fill(dx.begin(), dx.end(), -1);
        std::fill(dy.begin(), dy.end(), -1);

        for(int id: part1) {
            int u = idToIndex[id];
            if(xMatch[u] == -1) {
                q.push(u);
                dx[u] = 0;
            }
        }

        while(!q.empty()) {
            int u = q.front();
            q.pop();
            if(dx[u] > dis) {
                break;
            }

            auto edgeIdsExp = graph.getEdgeIdsOfVertex(vertices[u].id);
            if(!edgeIdsExp) {
                throw std::runtime_error("Failed to get edge ids of vertex");
            }
            const auto edgeIds = *edgeIdsExp;

            for(int edgeId: edgeIds) {
                const auto& edge = graph.getEdge(edgeId);
                int v = idToIndex[edge.to];

                if(dy[v] == -1) {
                    dy[v] = dx[u] + 1;
                    if(yMatch[v] == -1) {
                        dis = dy[v];
                    } else {
                        dx[yMatch[v]] = dy[v] + 1;
                        q.push(yMatch[v]);
                    }
                }
            }
        }
        return dis != INF;
    };

    auto hkDFS = [&](auto&& self, int u) -> bool {
        auto edgeIdsExp = graph.getEdgeIdsOfVertex(vertices[u].id);
        if(!edgeIdsExp) {
            throw std::runtime_error("Failed to get edge ids of vertex");
        }
        const auto edgeIds = *edgeIdsExp;

        for(int edgeId: edgeIds) {
            const auto& edge = graph.getEdge(edgeId);
            int v = idToIndex[edge.to];

            if(!vis[v] && dy[v] == dx[u] + 1) {
                vis[v] = true;
                if(yMatch[v] != -1 && dy[v] == dis) {
                    continue;
                }
                if(yMatch[v] == -1 || self(self, yMatch[v])) {
                    xMatch[u] = v;
                    yMatch[v] = u;
                    return true;
                }
            }
        }
        return false;
    };

    auto hkMain = [&]() -> int {
        int res = 0;
        while(hkBFS()) {
            std::fill(vis.begin(), vis.end(), false);
            for(int id: part1) {
                int u = idToIndex[id];
                if(xMatch[u] == -1) {
                    if(hkDFS(hkDFS, u)) {
                        res++;
                    }
                }
            }
        }
        return res;
    };

    hkMain();

    // 构建匹配映射
    std::unordered_map<int, int> matchMap;
    for(int i: part1) {
        int u = idToIndex[i];
        if(xMatch[u] != -1) {
            matchMap[i] = vertices[xMatch[u]].id;
        }
    }
    for(int i: part2) {
        int v = idToIndex[i];
        if(yMatch[v] != -1) {
            matchMap[i] = vertices[yMatch[v]].id;
        }
    }

    return matchMap;
}

}  // namespace GraphLib::algorithm
