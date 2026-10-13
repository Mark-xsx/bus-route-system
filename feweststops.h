#pragma once
#include <string>
#include "datamanager.h"
using namespace std;
//接下来写最少站，本质上可以用不做状态扩展的dijkstra算法，设置全部边权为1，但bfs最短路即可解决这个问题
class feweststops {
public:
	feweststops(datamanager& dm) :dm(dm) {}//构造时，把传进来的 datamanager&绑定到成员变量 dm 上。
	routeans find(const string& from, const string& to);//调用：routeans ans=st.find(,);
private:
	datamanager& dm;
	int transfercost = 5;
};
//如果有多条相同站数的结果，该怎么办，我认为保留额外副产物，时间，是个好想法，怎么实现，你发现
//突然想到如果shortest中也发生这个怎么办。我想知道这几种情况的发生可能性。
//1.shorteset 排列优先级（时间，换乘数）//实际应用，同样的时间，不换乘更舒适
//2.feweststops 排列优先级（站点数，时间）
//3.fewesttransfers 排列优先级（换乘数，时间）
//好了，要改上一个算法代码了//简而言之是代价的扩展
//我去，发现所有三个算法都可以用dijkstra算法解决，只是代价的设置和计算略有差别
//干了