#include "datamanager.h"

void datamanager::rebuildgraph()
{
	adjgraph.clear();
	allstations.clear();
	for (auto& item : alllines) {
		string lineid = item.first;//这里的键，用来快速找到busline,一般与内部是一致的
		busline& line = item.second;
		auto& stlist = line.stations;
		if (stlist.empty()) continue;//stlist可能为空
		for (int i = 0;i < stlist.size() - 1;i++) {//默认不考虑单站点线路，因为模拟的是南京地铁；
			station u = stlist[i];
			station v = stlist[i + 1];
			int cost = line.intervaltime[i];
			adjgraph[u].push_back({ v,{lineid,cost} });
			adjgraph[v].push_back({ u,{lineid,cost} });
			allstations.insert(u);//
			allstations.insert(v);//
		}
		//也可以仅保留下面两行，而注释掉上面两行，可我觉得下面用不着，上面的可读性更强
		//for(auto& s:stlist)
			//allstations.insert(s);//单站点线路也能收进来——基本无用
	}
}
bool datamanager::savetofile(string filepath)
{
	ofstream out(filepath);
	if (!out.is_open()) return false;

	//先写线路总数，便于加载时校验
	out << alllines.size() << '\n';//使用‘\n’不使用endl，避免不断刷新缓冲区
	for (auto& p : alllines) {
		busline& line = p.second;
		out << p.first << '\n';
		out << line.firstbus << " " << line.lastbus << '\n';
		//站点：一行一个：避免名字里有空格时被拆开
		out << line.stations.size() << '\n';
		for (auto& s : line.stations)
			out << s.name << '\n';
		//相邻站点之间的行驶时长：整数用空格分割，最后换行
		out << line.intervaltime.size() << '\n';
		for (auto& t : line.intervaltime)
			out << t << " ";
		out << '\n';
	}
	out.flush();//刷新输出流的缓冲区
	if (!out.good()) return false;
	out.close();
	return true;
}
bool datamanager::loadfromfile(string filepath)
{
	ifstream in(filepath);
	if (!(in.is_open())) return false;//核心作用是在执行任何文件读取逻辑前，先确认文件流对象已经成功关联到目标物理文件

	int linecount;
	if (!(in >> linecount) || linecount < 0) return false;
	in.ignore(numeric_limits<streamsize>::max(), '\n');//核心作用是直接跳过输入流里剩下的所有无效字符，直到遇到换行符\n为止，彻底解决cin连续输入时的遗留垃圾字符干扰问题。
	alllines.clear();
	for (int i = 0;i < linecount;i++) {
		busline line;
		//线路编号
		string lineid;
		if (!getline(in, lineid)) return false;//从输入流对象in中读取一整行的字符，写入字符串变量lineid中,返回值做布尔判断.
		//首末班车
		if (!(in >> line.firstbus >> line.lastbus)) return false;
		in.ignore(numeric_limits<streamsize>::max(), '\n');
		//站点数量+逐个站点
		int stcount;
		if (!(in >> stcount) || stcount < 0) return false;
		in.ignore(numeric_limits<streamsize>::max(), '\n');

		line.stations.reserve(stcount);//提前为线路的站点集合预分配刚好足够的内存空间，完全避免后续动态添加站点时触发vector的自动扩容拷贝开销
		for (int j = 0;j < stcount;j++) {
			string name;
			if (!getline(in, name)) return false;
			line.stations.emplace_back(name);//等价与line.stations.push_back(station(name));
		}
		//相邻车站所需时间+逐个整数
		int itcount;
		if (!(in >> itcount) || itcount < 0) return false;
		line.intervaltime.reserve(itcount);
		for (int j = 0;j < itcount;j++)
		{
			int t;
			if (!(in >> t)) return false;
			line.intervaltime.push_back(t);
		}
		in.ignore(numeric_limits<streamsize>::max(), '\n');
		//补充intervaltime和stations数量校验
		if (!line.stations.empty() && line.intervaltime.size() != line.stations.size() - 1)
		{
			return false;
		}
		alllines[lineid] = line;
	}
	rebuildgraph();
	return true;
}