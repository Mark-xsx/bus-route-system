#include "routequery.h"
#include <algorithm>
#include <climits>
void routequery::computemetrics(routeinfo& info) {
	info.totalstops = info.path.size() - 1;//站数指段数
	info.totaltransfer = 0;
	info.totaltime = 0;
	string lastline = "";//上一站所在线路，用来判断是否换乘
	for (int i = 0;i < info.path.size();i++) {
		const string& lineid = info.path[i].first;
		const station& s = info.path[i].second;
		if (i > 0) {
			auto it = dm.adjgraph.find(info.path[i - 1].second);
			if (it != dm.adjgraph.end()) {
				for (auto& edge : it->second) {
					if (edge.first == s && edge.second.first == lineid) {//上一站到当前站可能有多条边（不同线路都经过这两站）要找到当前路径实际走的那条
						info.totaltime += edge.second.second;
						break;
					}
				}
			}
		}
		if (lastline != "" && lineid != lastline) {
			info.totaltransfer++;
			info.totaltime += 5;
		}
		lastline = lineid;
	}
}
void routequery::dfs(const station& cur, dfscontext& ctx) {
	if (ctx.rawpaths.size() >= ctx.maxresults) return;
	if (cur == ctx.goal) {
		routeinfo info;
		info.lines = ctx.lines;
		info.path = ctx.path;
		computemetrics(info);
		ctx.rawpaths.push_back(info);
		return;
	}
	auto it = dm.adjgraph.find(cur);
	if (it == dm.adjgraph.end()) return;
	for (auto& edge : it->second) {
		const station& next = edge.first;
		const string& nextline = edge.second.first;
		if (ctx.visited.count(next)) continue;//剪枝
		int newtransfer = ctx.transfercount;
		//换乘线路
		if (ctx.lastline != "" && nextline != ctx.lastline) newtransfer++;//换乘
		if (newtransfer > ctx.maxtransfer) continue;//剪枝
		int oldtransfer = ctx.transfercount;//保留这一次循环的信息
		string oldlastline = ctx.lastline;//保留，递归回来时更新
		bool addedline = false;
		//记录线路，可以合并，但会牺牲可读性
		if (ctx.lines.empty() || ctx.lines.back() != nextline){//空的或者lines里的最后一条线不等于当前线，说明换乘了
			ctx.lines.push_back(nextline);
			addedline = true;
		}
		ctx.visited.insert(next);//更新下一次循环的上一站点，不走回头路
		ctx.path.push_back({nextline,next});//更新下一次循环的线路
		ctx.transfercount = newtransfer;//更新
		ctx.lastline = nextline;//更新
		dfs(next, ctx);
		ctx.path.pop_back();//回溯
		ctx.visited.erase(next);//回溯
			if (addedline) ctx.lines.pop_back();
		ctx.transfercount = oldtransfer;
		ctx.lastline = oldlastline;
		//累了
	}
}
vector<routeinfo> routequery::findallroutes(const string& from, const string& to, int maxtransfer, int maxresults) {
	vector<routeinfo> results;
	station start(from), goal(to);
	if (dm.adjgraph.find(start) == dm.adjgraph.end()) return results;
	if (dm.adjgraph.find(goal) == dm.adjgraph.end()) return results;//找不到对应站点则返回空答案	
	if (start == goal) return results;
	//填写dfs的基础参数
	dfscontext ctx;
	ctx.goal = goal;
	ctx.maxtransfer = maxtransfer;
	ctx.maxresults = maxresults;
	ctx.visited.insert(start);
	ctx.path.push_back({ "",start });
	dfs(start, ctx);
	//去重
	//DFS 可能枚举出相同的路径:1.同一条边可能有多条边2.不同搜索顺序，可能生成相同路径
	set<string> seen;
	for (auto& info : ctx.rawpaths) {
		string key;
		for (auto& p : info.path) key += p.first + "|" + p.second.name + ";";//把路径转为一个字符串
		if (seen.count(key)) continue;// 核心去重
			seen.insert(key);
		results.push_back(info);
	}
	return results;
}