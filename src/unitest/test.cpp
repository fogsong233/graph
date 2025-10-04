#include "./algorithm.h"
#include "./data.h"
#include <gtest/gtest.h>
#include <print>

// 基础测试：空图
using namespace GraphLib;

TEST(GraphTest, GetEdgesOfNonexistentVertex) {
    using MVertex = Vertex<int>;
    Graph<MVertex> g;
    EXPECT_EQ(0, g.numVertices());
}

TEST(GraphTest, AddAndRemoveVertex) {
    using MVertex = Vertex<int>;
    Graph<MVertex> g;
    MVertex v1(1, 10);
    MVertex v2(2, 20);
    g.addVertex(v1);
    g.addVertex(v2);
    EXPECT_EQ(2, g.numVertices());

    g.delVertex(1);
    EXPECT_EQ(1, g.numVertices());
    EXPECT_FALSE(g.delVertex(3));  // 删除不存在的顶点
    EXPECT_TRUE(g.delVertex(2));
    EXPECT_EQ(0, g.numVertices());
}

// 基础测试：添加和删除边
TEST(GraphTest, AddAndRemoveEdge) {
    using MVertex = Vertex<int>;
    Graph<MVertex> g;
    MVertex v1(1, 10);
    MVertex v2(2, 20);
    g.addVertex(v1);
    g.addVertex(v2);
    Edge e1(1, 1, 2);
    Edge e2(2, 2, 1);
    g.addEdge(e1);
    EXPECT_EQ(1, g.numEdges());
    g.addEdge(e2);
    EXPECT_EQ(2, g.numEdges());
    g.delEdge(1);
    EXPECT_EQ(1, g.numEdges());
    EXPECT_FALSE(g.delEdge(3));  // 删除不存在的边
    EXPECT_TRUE(g.delEdge(2));
    EXPECT_EQ(0, g.numEdges());
}

// 基础测试：获取顶点的边
TEST(GraphTest, GetEdgesOfVertex) {
    using MVertex = Vertex<int>;
    Graph<MVertex> g;
    MVertex v1(1, 10);
    MVertex v2(2, 20);
    MVertex v3(3, 30);
    g.addVertex(v1);
    g.addVertex(v2);
    g.addVertex(v3);
    Edge e1(1, 1, 2);
    Edge e2(2, 2, 3);
    g.addEdge(e1);
    g.addEdge(e2);
    auto edgesOfV2 = g.getEdgeIdsOfVertex(2);
    ASSERT_TRUE(edgesOfV2.has_value());
    // directed graph
    EXPECT_EQ(1, edgesOfV2->size());
    auto edgeFromId = g.getEdge(edgesOfV2->at(0));
    EXPECT_EQ(2, edgeFromId.from);
    EXPECT_EQ(3, edgeFromId.to);
}

// 基础测试：获取顶点的数据
TEST(GraphTest, GetDataOfVertex) {
    using MVertex = Vertex<int>;
    Graph<MVertex> g;
    MVertex v1(1, 10);
    MVertex v2(2, 20);
    g.addVertex(v1);
    g.addVertex(v2);
    auto data1 = g.getDataOfVertex(1);
    ASSERT_TRUE(data1.has_value());
    EXPECT_EQ(10, **data1);
    auto data2 = g.getDataOfVertex(2);
    ASSERT_TRUE(data2.has_value());
    EXPECT_EQ(20, **data2);
    auto data3 = g.getDataOfVertex(3);
    EXPECT_FALSE(data3.has_value());  // 顶点不存在
}

// 测试 Tarjan 算法
TEST(GraphTest, Tarjan) {
    UndirectedGraph<Vertex<void>> g;
    for(int i = 1; i <= 5; i++) {
        g.addVertex(Vertex<void>(i));
    }
    g.addEdge(Edge(0, 1, 2));
    g.addEdge(Edge(2, 2, 3));
    auto cuts = GraphLib::algorithm::tarjan(g);
    std::sort(cuts.begin(), cuts.end());
    std::vector<int> expectedCuts = {2};
    EXPECT_EQ(expectedCuts, cuts);

    UndirectedGraph<Vertex<void>> g2;
    for(int i = 1; i <= 7; i++) {
        g2.addVertex(Vertex<void>(i));
    }
    g2.addEdge(Edge(0, 1, 2));
    g2.addEdge(Edge(2, 2, 3));
    g2.addEdge(Edge(4, 2, 4));
    g2.addEdge(Edge(6, 3, 5));
    g2.addEdge(Edge(8, 5, 6));
    g2.addEdge(Edge(10, 6, 7));
    g2.addEdge(Edge(12, 7, 5));
    auto cuts2 = GraphLib::algorithm::tarjan(g2);
    std::sort(cuts2.begin(), cuts2.end());
    std::vector<int> expectedCuts2 = {2, 3, 5};
    EXPECT_EQ(expectedCuts2, cuts2);
}

// 测试二分图判定
TEST(GraphTest, IsBipartite) {
    UndirectedGraph<Vertex<void>> g;
    for(int i = 1; i <= 5; i++) {
        g.addVertex(Vertex<void>(i));
    }
    g.addEdge(Edge(0, 1, 2));
    g.addEdge(Edge(2, 2, 3));
    g.addEdge(Edge(4, 3, 4));
    g.addEdge(Edge(6, 4, 5));
    auto partExp = GraphLib::algorithm::isBipartite(g);
    ASSERT_TRUE(partExp.has_value());
    auto part = *partExp;
    std::sort(part.begin(), part.end());
    std::vector<int> expectedPart = {1, 3, 5};
    EXPECT_EQ(expectedPart, part);
    // 添加一条边使图变为非二分图
    g.addEdge(Edge(8, 1, 3));
    auto partExp2 = GraphLib::algorithm::isBipartite(g);
    EXPECT_FALSE(partExp2.has_value());
    EXPECT_EQ("The graph is not bipartite", partExp2.error());
}

// 测试 Hopcraft-Karp 算法求最大匹配
TEST(GraphTest, HopcraftKarp) {
    UndirectedGraph<Vertex<void>> g;
    for(int i = 1; i <= 6; i++) {
        g.addVertex(Vertex<void>(i));
    }
    g.addEdge(Edge(0, 1, 4));
    g.addEdge(Edge(2, 1, 5));
    g.addEdge(Edge(4, 2, 5));
    g.addEdge(Edge(6, 3, 6));
    g.addEdge(Edge(8, 2, 6));
    std::vector<int> part1 = {1, 2, 3};
    auto matchMap = GraphLib::algorithm::getMaxMatchByHopcraftKarp(g, part1);

    std::unordered_map<int, int> expectedMatchMap = {
        {1, 4},
        {2, 5},
        {3, 6},
        {4, 1},
        {5, 2},
        {6, 3},
    };
    EXPECT_EQ(expectedMatchMap, matchMap);
}

// 距离
TEST(GraphTest, BFS) {
    UndirectedGraph<Vertex<void>> g;
    for(int i = 1; i <= 6; i++) {
        g.addVertex(Vertex<void>(i));
    }
    g.addEdge(Edge(0, 1, 2));
    g.addEdge(Edge(2, 1, 3));
    g.addEdge(Edge(4, 2, 4));
    g.addEdge(Edge(6, 3, 5));
    g.addEdge(Edge(8, 4, 6));

    auto distExp = GraphLib::algorithm::distanceWithoutWeight(g, 1, 6);
    EXPECT_EQ(3, distExp);
    auto distExp2 = GraphLib::algorithm::distanceWithoutWeight(g, 1, 5);
    EXPECT_EQ(2, distExp2);
}

// 子图
TEST(GraphTest, Subgraph) {
    using MVertex = Vertex<int>;
    Graph<MVertex> g;
    for(int i = 1; i <= 5; i++) {
        g.addVertex(MVertex(i, i * 10));
    }
    g.addEdge(Edge(1, 1, 2));
    g.addEdge(Edge(2, 2, 3));
    g.addEdge(Edge(3, 3, 4));
    g.addEdge(Edge(4, 4, 5));
    g.addEdge(Edge(5, 1, 5));
    // 顶点子图
    GraphLib::Graph<Vertex<int>> subG1 = g.subgraphOfVertices({1, 2, 3});

    EXPECT_EQ(3, subG1.numVertices());
    EXPECT_EQ(2, subG1.numEdges());
    EXPECT_EQ(10, subG1.getVertex(1).data);
    EXPECT_EQ(20, subG1.getVertex(2).data);
    EXPECT_EQ(30, subG1.getVertex(3).data);
    // 边子图
    GraphLib::Graph<Vertex<int>> subG2 = g.subgraphOfEdges({2, 3, 5});
    EXPECT_EQ(3, subG2.numVertices());
    std::println("Subgraph2: {}", subG2);
    EXPECT_EQ(3, subG2.numEdges());
    EXPECT_EQ(20, subG2.getVertex(2).data);
    EXPECT_EQ(30, subG2.getVertex(3).data);
    EXPECT_EQ(50, subG2.getVertex(5).data);
}
