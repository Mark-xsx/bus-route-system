#include "shortesttime.h"
#include <queue>
#include <map>
#include <vector>
#include <algorithm>
routeans shortesttime::find(const string& from, const string& to)
{
	routeans ans;
	ans.totalstops = 0;
	ans.totaltransfer = 0;
	ans.totaltime = 0;
	station start(from), goal(to);//把from和to的字符串赋值给start.name和goal.name.
	if (dm.adjgraph.find(start) == dm.adjgraph.end()) return ans;
	if (dm.adjgraph.find(goal) == dm.adjgraph.end()) return ans;
	//状态扩展
	typedef pair<station, string> state;//(站点，当前线路id)
	typedef pair<int, int> cost;//代价的扩展（时间，换乘次数）,每次循环进入寻找的最小代价是排列优先级（时间，换乘数
	map<state, cost> dist;//到某状态的的最小代价
	map<state, state> prev;//记录上一个站点，用于回溯路径
	map<state, int> trans;//记录换乘次数
	map<state, bool> st;//记录是否已确认了最短路
	typedef pair<cost, state> item;//(累计代价，状态)
	priority_queue<item, vector<item>, greater<item>> heap;//小根堆
	state startstate = { start,"" };//起始状态
	dist[startstate] = { 0,0 };
	trans[startstate] = 0;
	heap.push({ {0,0},startstate });
	state goalstate;
	bool found = false;
	//djistra算法核心
	while (!heap.empty()) {
		item cur = heap.top();heap.pop();
		cost curcost = cur.first;
		int curtime = curcost.first;
		int curtrans = curcost.second;
		state s = cur.second;
		if (st[s]) continue;
		st[s] = true;
		if (s.first == goal) {
			goalstate = s;
			found = true;
			break;
		}
		auto it = dm.adjgraph.find(s.first);
		if (it == dm.adjgraph.end()) continue;//如果找不到该站点，下一次循环进入
		for (auto& edge : it->second) {//it->second为adjgraph[s.first]的第一个键值对的值
			const station& v = edge.first;
			const string& nextline = edge.second.first;
			int nexttime = edge.second.second;
			int newtime = curtime + nexttime;
			int newtrans = curtrans;
			if (s.second == "" || s.second == nextline) {//起点或不需换乘则继续
				//
			}
			else {
				newtime += transfercost;
				newtrans += 1;
			}
			cost newcost = { newtime,newtrans };
			state newstate = { v,nextline };
			auto dit = dist.find(newstate);
			//一般dijkstra:dist[]全部初始化为INF,但这里是map存储不存在初始化，所以第一次更新时，直接添加
			if (dit == dist.end() || newcost < dit->second) {//进行松弛操作，更新出下一轮。
				dist[newstate] = newcost;
				trans[newstate] = newtrans;
				prev[newstate] = s;
				heap.push({ newcost,newstate });
			}
		}
	}
	if (!found) return ans;//如若没有找到对应的终点站，则goalstate一次没更新，此时ans仍然都为0
	//回溯路径
	vector<state> path;
	state cur = goalstate;
	while (true) {
		path.push_back(cur);
		if (cur == startstate) break;
		cur = prev[cur];
	}
	reverse(path.begin(), path.end());//倒转后才是真正的顺序
	//最后更新答案
	ans.totaltime = dist[goalstate].first;
	ans.totaltransfer = dist[goalstate].second;
	ans.totalstops = path.size() - 1;
	for (auto& s : path) {
		ans.routepath.push_back({ s.second,s.first });
	}
	return ans;
}