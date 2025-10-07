## libGraph
### build & test
使用[Xmake](Xmake.io)构建, 在MacOS, Wsl2的clang编译器
上成功构建, 依赖gtest单元测试, 版本是cpp23.
### Feature
编译期多态(Template), 对于算法和不同的Vertax的data
在编译期就可以确定, 完全没有必要virtual.

算法是函数实现, 因为返回值是不确定的, 各个算法完全
不一样, 强行抽象是无法得到任何统一性, 但是函数是
模板函数, 支持图的多态.

储存数据和实现分离, 而且对于不能修改的数据使用const,
而非全部private然后提供get方法. 对于能修改的属性应当
直接暴露, 因为现在没有约束数据实现的方法, 当然
这里Vertax是多态的, 完全可以自己实现get, 然后让
Graph类作为friend class.

高度的扩展性, 无侵入, 正确性可以查看test.