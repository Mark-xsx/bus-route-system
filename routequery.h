#pragma once
#include <string>
#include <vector>
#include <set>
#include "datamanager.h"
using namespace std;
//现在实现了三种查询最佳的线路的算法，但难以实现输出所有线路的问题，因为djikstra，一但一条单源最短路
//与相应的代价锁定了，就直接退出了循环，不会更新出次好的。因此需要dfs寻找多条路径（最多不超过20条）
//按时间排序取出前六条，再与最佳路线合并（兜底），最后按标签为首要，时间为次要排序；
//模拟高德地图//做成快速模式和详细模式，创新点
//一条线路的完整信息
struct routeinfo {
	vector<string> lines;//线路序列
	vector<pair<string, station>> path;//完整站点序列（线路，站点）
	int totalstops = 0;
	int totaltransfer = 0;
	int totaltime = 0;
	//兜底标记：这条路线是不是某个算法算出的最优
	bool isbesttime = false;
	bool isbeststops = false;
	bool isbesttrans = false;
};
class routequery {
public:
	routequery(datamanager& dm) :dm(dm) {}
	//枚举所有可达路线，限制最大换乘数和最多二十条线路，一般不可能达到。
	vector<routeinfo> findallroutes(const string& from, const string& to, int maxtransfer = 3, int maxresults=20);
private:
	datamanager& dm;
	struct dfscontext {
		station goal;
		set<station> visited;//不走回头路，标记当前站点的上一个站点，剪枝。
		vector<string> lines;//存储依次坐过的线路
		vector<pair<string, station>> path;//存储站点（路线，站台）
		vector<routeinfo> rawpaths;//存储所有的路线信息
		int maxtransfer;
		int maxresults;
		int transfercount = 0;
		string lastline = "";
	};
	void dfs(const station& cur, dfscontext& ctx);//找路，得到一条路的完整线路和站点信息
	void computemetrics(routeinfo& info);//计算对应线路的所有指标  	
};