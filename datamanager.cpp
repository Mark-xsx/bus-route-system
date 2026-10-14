#include "datamanager.h"

// ==================== 重建邻接图 ====================
// 根据 alllines 中的线路数据，重新构建 adjgraph 和 allstations
void datamanager::rebuildgraph()
{
    adjgraph.clear();
    allstations.clear();
    for (auto& item : alllines) {
        string lineid = item.first;       // 线路编号
        busline& line = item.second;      // 该线路的完整信息
        auto& stlist = line.stations;     // 站点列表
        if (stlist.empty()) continue;     // 空线路跳过
        for (int i = 0; i < (int)stlist.size() - 1; i++) {
            station u = stlist[i];
            station v = stlist[i + 1];
            int cost = line.intervaltime[i];
            // 无向图：两个方向都加边
            adjgraph[u].push_back({ v, {lineid, cost} });
            adjgraph[v].push_back({ u, {lineid, cost} });
            allstations.insert(u);
            allstations.insert(v);
        }
    }
}

// ==================== 保存到文件 ====================
// 把 alllines 按固定格式写入 filepath
bool datamanager::savetofile(string filepath)
{
    ofstream out(filepath);
    if (!out.is_open()) return false;

    // 第一行：线路总数
    out << alllines.size() << '\n';
    for (auto& p : alllines) {
        busline& line = p.second;
        out << p.first << '\n';                              // 线路编号
        out << line.firstbus << " " << line.lastbus << '\n'; // 首末班车
        // 站点：一行一个
        out << line.stations.size() << '\n';
        for (auto& s : line.stations)
            out << s.name << '\n';
        // 相邻站点之间的行驶时长：空格分隔
        out << line.intervaltime.size() << '\n';
        for (auto& t : line.intervaltime)
            out << t << " ";
        out << '\n';
    }
    out.flush();
    if (!out.good()) return false;
    out.close();
    return true;
}

// ==================== 从文件加载 ====================
// 读取 data.txt 并填充 alllines，最后重建邻接图
// 注意：兼容 Windows 的 CRLF 换行，去掉每行末尾的 '\r'
bool datamanager::loadfromfile(string filepath)
{
    ifstream in(filepath);
    if (!in.is_open()) return false;

    int linecount;
    if (!(in >> linecount) || linecount < 0) return false;
    in.ignore(numeric_limits<streamsize>::max(), '\n');

    alllines.clear();
    for (int i = 0; i < linecount; i++) {
        busline line;

        // 线路编号：去掉行尾 \r
        string lineid;
        if (!getline(in, lineid)) return false;
        if (!lineid.empty() && lineid.back() == '\r') lineid.pop_back();

        // 首末班车
        if (!(in >> line.firstbus >> line.lastbus)) return false;
        in.ignore(numeric_limits<streamsize>::max(), '\n');

        // 站点数量
        int stcount;
        if (!(in >> stcount) || stcount < 0) return false;
        in.ignore(numeric_limits<streamsize>::max(), '\n');

        // 逐个站点：去掉行尾 \r
        line.stations.reserve(stcount);
        for (int j = 0; j < stcount; j++) {
            string name;
            if (!getline(in, name)) return false;
            if (!name.empty() && name.back() == '\r') name.pop_back();
            line.stations.emplace_back(name);
        }

        // 相邻车站所需时间
        int itcount;
        if (!(in >> itcount) || itcount < 0) return false;
        line.intervaltime.reserve(itcount);
        for (int j = 0; j < itcount; j++) {
            int t;
            if (!(in >> t)) return false;
            line.intervaltime.push_back(t);
        }
        in.ignore(numeric_limits<streamsize>::max(), '\n');

        // 校验：站间时间个数 = 站点数 - 1
        if (!line.stations.empty()
            && line.intervaltime.size() != line.stations.size() - 1) {
            return false;
        }

        alllines[lineid] = line;
    }

    rebuildgraph();
    return true;
}
