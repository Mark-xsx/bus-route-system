#pragma once
#include<iostream>
#include <string>
#include <vector>//vector 动态数组
#include <map>//alllines：线路编号→线路信息adjgraph：站点→相邻站点边集合
#include <set>//set 有序不重复集合,allstations，保存全部站点，自动剔除重复站点
#include <fstream>//loadfromfile读取 data.txt；savetofile保存文件
#include <limits>//numeric_limits<streamsize>::max()，配合in.ignore()，吃掉换行残留字符
#include <utility>//pair 键值对结构体
using namespace std;
// 公交站结构体：存储单个站点信息
struct station{
	string name;
	station(string n = "") :name(n) {}
	bool operator<(const station& other) const {
		return name < other.name;
	}
	bool operator==(const station& other) const {
		return name == other.name;
	}
};
// 公交线路结构体：存储单条完整线路全量信息
struct busline {
	//注意不再保存lineid,统一用alllines的键作为线路编号,避免键与字段不一致
	vector<station> stations;
	vector<int> intervaltime;
	string firstbus;
	string lastbus;
};
// 路径结果结构体：封装查询返回的完整路线信息
struct routeans {
	vector<pair<string, station>> routepath;
	int totalstops;
	int totaltransfer;
	int totaltime;
};
class datamanager {
public://(封装全局变量)
	map<string, busline> alllines;//键 = 线路编号
	map<station, vector<pair<station, pair<string, int>>>> adjgraph;//站点邻接图
	set<station> allstations;//去重后的站点
	//重建邻接图
	void rebuildgraph();//根据更改的alllines 来修改adjgraph和allstations;
	bool loadfromfile(string filepath);
	bool savetofile(string filepath);
};