#pragma once

#include <algorithm>
#include <expected>
#include <format>
#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace GraphLib {

template <typename T, typename CharT = char>
concept Formattable = requires(T t, std::format_context ctx) { typename std::formatter<T, CharT>; };

class UndirectedEdgeIdCounter {
public:
    int getNextId() {
        currentId += 2;
        return currentId;
    };

    static int getPairedId(int id) {
        if(id % 2 == 0) {
            return id - 1;
        } else {
            return id + 1;
        }
    };

private:
    int currentId = 0;
};

struct VertexBase {
    const int id;

    VertexBase(int id) : id(id) {}

    VertexBase() : id(-1) {}

    std::string toString() const {
        return std::format("vertex<{}>", id);
    }

    bool operator== (const VertexBase& rhs) const {
        return id == rhs.id;
    }
};

template <typename DataTy>
struct Vertex : public VertexBase {
    DataTy data;
    using VertexBase::VertexBase;

    Vertex(int id, const DataTy d) : VertexBase(id), data(std::move(d)) {}

    Vertex<DataTy>(const Vertex<DataTy>& other) : VertexBase(other.id), data(other.data) {}

    [[nodiscard]] std::string toString() const {
        if constexpr(Formattable<DataTy>) {
            return std::format("vertex<{}: {}>", id, data);
        }
        return VertexBase::toString();
    }
};

template <>
struct Vertex<void> : public VertexBase {
    using VertexBase::VertexBase;
};

using TrivialVertex = Vertex<void>;

template <typename V>
struct _isVertex : std::false_type {};

template <typename DataTy>
struct _isVertex<Vertex<DataTy>> : std::true_type {};

template <typename V>
concept isVertex = _isVertex<V>::value;

struct Edge {
    const int id;
    int from;
    int to;
    int weight;

    Edge() : id(-1), from(-1), to(-1), weight(1) {}

    Edge(int id, int from, int to, int weight = 1) : id(id), from(from), to(to), weight(weight) {}

    bool operator== (const Edge& rhs) const {
        return id == rhs.id;
    }

    std::string toString() const {
        return std::format("edge<{}: {} -> {}>", id, from, to);
    }
};

// 自定义哈希器（避免 std::hash 部分特化非法）
template <typename V>
struct VertexHasher {
    size_t operator() (const V& v) const noexcept {
        return std::hash<int>()(v.id);
    }
};

struct EdgeHasher {
    size_t operator() (const Edge& e) const noexcept {
        return std::hash<int>()(e.id);
    }
};

template <isVertex V>
struct GraphData {
    using AdjMap = std::unordered_map<int, std::unordered_set<int>>;
    AdjMap adjMap;  // vertex id -> {edge ids}
    std::unordered_map<int, V> idToVertex;
    std::unordered_map<int, Edge> idToEdge;

    GraphData() {}
};

template <isVertex V>
class Graph {
    template <typename VV>
    struct VertexData {
        using type = std::remove_reference_t<decltype(std::declval<VV>().data)>;
    };

    template <>
    struct VertexData<Vertex<void>> {
        using type = void;
    };

public:
    using VertexDataTy = typename VertexData<V>::type;

    Graph() {};

    Graph(GraphData<V> data) : data(std::move(data)) {}

    Graph(Graph&) = delete;

    virtual void addVertex(const V& v) {
        data.adjMap[v.id];
        data.idToVertex.emplace(v.id, v);
    }

    virtual bool addEdge(const Edge& e) {
        data.adjMap[e.from].insert(e.id);
        data.idToEdge.emplace(e.id, e);
        return true;
    }

    virtual bool delEdge(int id) {
        auto it = data.idToEdge.find(id);
        if(it == data.idToEdge.end())
            return false;
        const auto& edge = it->second;
        if(data.adjMap.count(edge.from))
            data.adjMap[edge.from].erase(id);
        data.idToEdge.erase(it);
        return true;
    }

    virtual bool delVertex(int id) {
        bool erased = false;
        auto it = data.adjMap.find(id);
        if(it != data.adjMap.end()) {
            for(auto edgeId: it->second)
                data.idToEdge.erase(edgeId);
            data.adjMap.erase(it);
            data.idToVertex.erase(id);
            erased = true;
        }
        for(auto& [v, edges]: data.adjMap) {
            std::vector<int> removeList;
            for(auto edgeId: edges) {
                if(data.idToEdge.at(edgeId).to == id) {
                    removeList.push_back(edgeId);
                }
            }
            for(auto eid: removeList) {
                edges.erase(eid);
                data.idToEdge.erase(eid);
                erased = true;
            }
        }
        return erased;
    }

    [[nodiscard]] int numVertices() const {
        return data.adjMap.size();
    }

    [[nodiscard]] virtual int numEdges() const {
        return data.idToEdge.size();
    }

    // getDataOfVertex：仅当 V 不是 Vertex<void> 时可用
    [[nodiscard]] std::expected<const VertexDataTy*, int> getDataOfVertex(int id) const {
        static_assert(!std::is_same_v<V, Vertex<void>>, "Vertex<void> has no data");
        auto it = data.idToVertex.find(id);
        if(it == data.idToVertex.end()) {
            return std::unexpected(-1);
        }
        return std::expected<const VertexDataTy*, int>(&(it->second.data));
    }

    std::expected<std::vector<int>, int> getEdgeIdsOfVertex(int id) const {
        auto it = data.adjMap.find(id);
        if(it == data.adjMap.end())
            return std::unexpected(-1);
        std::vector<int> edgeIds;
        edgeIds.reserve(it->second.size());
        for(auto edgeId: it->second)
            edgeIds.push_back(edgeId);
        return edgeIds;
    }

    std::vector<V> getAllVertices() const {
        std::vector<V> vertices;
        for(const auto& [id, vertex]: data.idToVertex) {
            vertices.push_back(vertex);
        }
        return vertices;
    }

    GraphData<V> subgraphOfVertices(const std::vector<int>& ids) const {
        auto adjMap = lightSubgraphOfVertices(ids);
        GraphData<V> subData;
        subData.adjMap = std::move(adjMap);
        for(const auto& [id, _]: subData.adjMap) {
            subData.idToVertex.emplace(id, data.idToVertex.at(id));
        }
        for(const auto& [id, edges]: subData.adjMap) {
            for(auto eid: edges) {
                subData.idToEdge.emplace(eid, data.idToEdge.at(eid));
            }
        }
        return subData;
    }

    GraphData<V> subgraphOfEdges(const std::vector<int>& ids) const {
        auto adjMap = lightSubgraphOfEdges(ids);
        GraphData<V> subData;
        subData.adjMap = std::move(adjMap);
        for(const auto& [id, edges]: subData.adjMap) {
            subData.idToVertex.emplace(id, data.idToVertex.at(id));
            for(auto eid: edges) {
                auto& ed = data.idToEdge.at(eid);
                subData.idToEdge.emplace(eid, ed);
                subData.idToVertex.emplace(ed.to, data.idToVertex.at(ed.to));
            }
        }
        return subData;
    }

    typename GraphData<V>::AdjMap lightSubgraphOfVertices(const std::vector<int>& ids) const {
        std::unordered_set<int> idSet(ids.begin(), ids.end());
        typename GraphData<V>::AdjMap lightSubMap;
        for(const auto& [id, edges]: data.adjMap) {
            if(!idSet.count(id))
                continue;
            lightSubMap[id];
            for(auto eid: edges) {
                const auto& edge = data.idToEdge.at(eid);
                if(idSet.count(edge.to)) {
                    lightSubMap[id].insert(eid);
                }
            }
        }
        return lightSubMap;
    }

    typename GraphData<V>::AdjMap lightSubgraphOfEdges(const std::vector<int>& ids) const {
        std::unordered_set<int> idSet(ids.begin(), ids.end());
        typename GraphData<V>::AdjMap lightSubMap;
        for(const auto& [id, edges]: data.adjMap) {
            for(auto eid: edges) {
                if(!idSet.count(eid))
                    continue;
                const auto& edge = data.idToEdge.at(eid);
                lightSubMap[edge.from].insert(eid);
            }
        }
        return lightSubMap;
    }

    const V& getVertex(int id) const {
        return this->data.idToVertex.at(id);
    }

    const Edge& getEdge(int id) const {
        return this->data.idToEdge.at(id);
    }

protected:
    GraphData<V> data;
    friend std::formatter<GraphLib::Graph<V>>;
};

template <isVertex V>
class UndirectedGraph : public Graph<V> {
public:
    using Graph<V>::Graph;

    bool addEdge(const Edge& e) override {
        this->data.adjMap[e.from].insert(e.id);
        this->data.idToEdge.emplace(e.id, e);
        Edge revEdge(UndirectedEdgeIdCounter::getPairedId(e.id), e.to, e.from, e.weight);
        this->data.idToEdge.emplace(revEdge.id, revEdge);
        this->data.adjMap[e.to].insert(revEdge.id);
        return true;
    }

    bool delEdge(int id) override {
        auto it = this->data.idToEdge.find(id);
        if(it == this->data.idToEdge.end())
            return false;
        const auto& e = it->second;
        this->data.adjMap[e.from].erase(id);
        this->data.idToEdge.erase(it);
        int pairedId = UndirectedEdgeIdCounter::getPairedId(id);
        auto it2 = this->data.idToEdge.find(pairedId);
        if(it2 != this->data.idToEdge.end()) {
            const auto& e2 = it2->second;
            this->data.adjMap[e2.from].erase(it2->first);
        }
        return true;
    }

    int numEdges() const override {
        return this->data.idToEdge.size() / 2;
    }

    GraphData<V> complement() const {
        typename GraphData<V>::AdjMap compMap;
        GraphData<V> graphData;
        graphData.idToVertex = this->data.idToVertex;
        UndirectedEdgeIdCounter idCounter;
        for(const auto& p: this->data.idToEdge)

            for(const auto& [v, _]: this->data.adjMap) {
                for(const auto& [u, _2]: this->data.adjMap) {
                    if(u == v)
                        continue;
                    bool connected = false;
                    for(auto eid: this->data.adjMap.at(v)) {
                        if(this->data.idToEdge.at(eid).to == u) {
                            connected = true;
                            break;
                        }
                    }
                    if(!connected && !compMap[v].count(u)) {
                        Edge e(idCounter.getNextId(), v, u);
                        Edge revE(UndirectedEdgeIdCounter::getPairedId(e.id), u, v);
                        compMap[v].insert(e.id);
                        compMap[u].insert(e.id);
                        graphData.idToEdge[e.id] = e;
                        graphData.idToEdge[revE.id] = revE;
                    }
                }
            }
        graphData.adjMap = std::move(compMap);
        return graphData;
    }

    friend std::formatter<GraphLib::Graph<V>>;
};

}  // namespace GraphLib

// formatter implementations
template <>
struct std::formatter<GraphLib::VertexBase> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const GraphLib::VertexBase& v, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "{}", v.toString());
    }
};

template <>
struct std::formatter<GraphLib::Edge> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const GraphLib::Edge& e, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "{}", e.toString());
    }
};

template <typename DataTy>
struct std::formatter<GraphLib::Vertex<DataTy>> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const GraphLib::Vertex<DataTy>& v, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "{}", v.toString());
    }
};

// Custom formatter for unordered_map<int, Edge>
template <>
struct std::formatter<std::unordered_map<int, GraphLib::Edge>> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const std::unordered_map<int, GraphLib::Edge>& edges, FormatContext& ctx) const {
        auto out = ctx.out();
        *out++ = '[';
        bool first = true;
        for(const auto& [id, edge]: edges) {
            if(!first) {
                *out++ = ',';
                *out++ = ' ';
            }
            first = false;
            out = std::format_to(out, "{}", edge);
        }
        *out++ = ']';
        return out;
    }
};

// Custom formatter for vector<Vertex<T>>
template <typename T>
struct std::formatter<std::vector<GraphLib::Vertex<T>>> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const std::vector<GraphLib::Vertex<T>>& vertices, FormatContext& ctx) const {
        auto out = ctx.out();
        *out++ = '[';
        bool first = true;
        for(const auto& vertex: vertices) {
            if(!first) {
                *out++ = ',';
                *out++ = ' ';
            }
            first = false;
            out = std::format_to(out, "{}", vertex);
        }
        *out++ = ']';
        return out;
    }
};

template <GraphLib::isVertex V, template <typename VV> class GraphImpl>
    requires std::is_base_of_v<GraphLib::Graph<V>, GraphImpl<V>>
struct std::formatter<GraphImpl<V>> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const GraphLib::Graph<V>& g, FormatContext& ctx) const {
        return std::format_to(ctx.out(),
                              "\n\nGraph<{} vertices, {} edges>\nvertices:\n {}\nedges:\n {}\n",
                              g.numVertices(),
                              g.numEdges(),
                              g.getAllVertices(),
                              g.data.idToEdge);
    }
};
