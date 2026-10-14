#include "menu.h"
#include "shortesttime.h"
#include "feweststops.h"
#include "fewesttransfers.h"
#include "routequery.h"
#include <iostream>
#include <limits>
#include <climits>
#include <algorithm>

using namespace std;

// ==================== Web 业务处理函数 ====================

// 登录
json handleLogin(const json& req) {
    string role = req.value("role", "");
    string pwd  = req.value("password", "");
    json out;
    if (role == "user") {
        out["success"] = true;
    } else if (role == "admin") {
        out["success"] = (pwd == "12345");
        if (!out["success"]) out["message"] = "管理员密码错误";
    } else {
        out["success"] = false;
        out["message"] = "未知角色";
    }
    return out;
}

// 线路查询（三种策略）
json handleRouteQuery(const json& req, datamanager& dm) {
    string from = req.value("from", "");
    string to   = req.value("to", "");
    int strategy = req.value("strategy", 1);

    routeans ans;
    if (strategy == 1) {
        shortesttime st(dm);
        ans = st.find(from, to);
    } else if (strategy == 2) {
        feweststops fs(dm);
        ans = fs.find(from, to);
    } else if (strategy == 3) {
        fewesttransfers ft(dm);
        ans = ft.find(from, to);
    } else {
        json err;
        err["success"] = false;
        err["message"] = "无效策略";
        return err;
    }

    json j;
    j["success"] = !ans.routepath.empty();
    j["totalstops"] = ans.totalstops;
    j["totaltransfer"] = ans.totaltransfer;
    j["totaltime"] = ans.totaltime;

    json steps = json::array();
    for (auto& p : ans.routepath) {
        json step;
        step["line"] = p.first;
        step["station"] = p.second.name;
        steps.push_back(step);
    }
    j["steps"] = steps;
    return j;
}

// 车次查询
json handleLineQuery(const json& req, datamanager& dm) {
    string lineid = req.value("lineid", "");
    auto it = dm.alllines.find(lineid);
    json out;
    if (it == dm.alllines.end()) {
        out["success"] = false;
        out["message"] = "未找到线路";
    } else {
        const busline& line = it->second;
        out["success"] = true;
        out["lineid"] = lineid;
        out["firstbus"] = line.firstbus;
        out["lastbus"] = line.lastbus;

        json stations = json::array();
        for (size_t i = 0; i < line.stations.size(); ++i) {
            json s;
            s["name"] = line.stations[i].name;
            if (i < line.intervaltime.size())
                s["interval"] = line.intervaltime[i];
            else
                s["interval"] = 0;
            stations.push_back(s);
        }
        out["stations"] = stations;
    }
    return out;
}

// 管理员：新增线路
json handleAddLine(const json& req, datamanager& dm) {
    string lineid = req.value("lineid", "");
    json out;
    if (dm.alllines.count(lineid)) {
        out["success"] = false;
        out["message"] = "线路已存在";
        return out;
    }

    busline line;
    line.firstbus = req.value("firstbus", "");
    line.lastbus  = req.value("lastbus", "");

    for (auto& s : req["stations"]) {
        string name = s.value("name", "");
        int interval = s.value("interval", 0);
        line.stations.emplace_back(name);
        line.intervaltime.push_back(interval);
    }
    if (!line.intervaltime.empty()) line.intervaltime.pop_back();

    dm.alllines[lineid] = line;
    dm.rebuildgraph();
    bool ok = dm.savetofile("data.txt");

    out["success"] = ok;
    out["message"] = ok ? "新增成功" : "保存失败";
    return out;
}

// 管理员：修改线路
json handleModifyLine(const json& req, datamanager& dm) {
    string lineid = req.value("lineid", "");
    auto it = dm.alllines.find(lineid);
    json out;
    if (it == dm.alllines.end()) {
        out["success"] = false;
        out["message"] = "线路不存在";
        return out;
    }

    busline& line = it->second;
    line.firstbus = req.value("firstbus", "");
    line.lastbus  = req.value("lastbus", "");
    line.stations.clear();
    line.intervaltime.clear();

    for (auto& s : req["stations"]) {
        string name = s.value("name", "");
        int interval = s.value("interval", 0);
        line.stations.emplace_back(name);
        line.intervaltime.push_back(interval);
    }
    if (!line.intervaltime.empty()) line.intervaltime.pop_back();

    dm.rebuildgraph();
    bool ok = dm.savetofile("data.txt");

    out["success"] = ok;
    out["message"] = ok ? "修改成功" : "保存失败";
    return out;
}

// 获取所有线路名
json handleGetLines(datamanager& dm) {
    json arr = json::array();
    for (auto& p : dm.alllines) arr.push_back(p.first);
    return arr;
}

// 所有可达路线（详细模式）
json handleAllRoutes(const json& req, datamanager& dm) {
    string from = req.value("from", "");
    string to   = req.value("to", "");
    int maxTransfer = req.value("maxTransfer", 3);
    int maxResults  = req.value("maxResults", 20);

    // DFS 枚举
    routequery rq(dm);
    vector<routeinfo> all = rq.findallroutes(from, to, maxTransfer, maxResults);

    // 三个算法兜底，打标记
    shortesttime st(dm);
    feweststops fs(dm);
    fewesttransfers ft(dm);
    mergeroute(all, st.find(from, to), 1);
    mergeroute(all, fs.find(from, to), 2);
    mergeroute(all, ft.find(from, to), 3);

    json out;
    if (all.empty()) {
        out["success"] = false;
        out["message"] = "两站之间不可达";
        return out;
    }

    // 找最优值
    int bestT = INT_MAX, bestS = INT_MAX, bestX = INT_MAX;
    for (auto& r : all) {
        bestT = min(bestT, r.totaltime);
        bestS = min(bestS, r.totalstops);
        bestX = min(bestX, r.totaltransfer);
    }

    // 排序：标签多的在前，时间少的在前
    sort(all.begin(), all.end(),
        [&](const routeinfo& a, const routeinfo& b) {
            int ca = 0, cb = 0;
            if (a.totaltime == bestT)     ca++;
            if (a.totalstops == bestS)    ca++;
            if (a.totaltransfer == bestX) ca++;
            if (b.totaltime == bestT)     cb++;
            if (b.totalstops == bestS)    cb++;
            if (b.totaltransfer == bestX) cb++;
            if (ca != cb) return ca > cb;
            if (a.totaltime != b.totaltime) return a.totaltime < b.totaltime;
            return a.totalstops < b.totalstops;
        });

    out["success"] = true;
    out["bestT"] = bestT;
    out["bestS"] = bestS;
    out["bestX"] = bestX;

    json routes = json::array();
    for (auto& r : all) {
        json jr;
        jr["lines"] = r.lines;
        jr["totalstops"] = r.totalstops;
        jr["totaltransfer"] = r.totaltransfer;
        jr["totaltime"] = r.totaltime;
        jr["isbesttime"] = (r.totaltime == bestT);
        jr["isbeststops"] = (r.totalstops == bestS);
        jr["isbesttrans"] = (r.totaltransfer == bestX);

        json steps = json::array();
        for (auto& p : r.path) {
            json step;
            step["line"] = p.first;
            step["station"] = p.second.name;
            steps.push_back(step);
        }
        jr["steps"] = steps;
        routes.push_back(jr);
    }
    out["routes"] = routes;
    return out;
}

// ==================== 控制台菜单函数（保留，Web 版不调用） ====================

void printroute(const routeans& ans)
{
    if (ans.routepath.empty()) {
        cout << "未找到路线\n";
        return;
    }
    cout << "\n=====路线=====\n";
    string curline = "";
    for (auto& p : ans.routepath) {
        const string& lineid = p.first;
        const station& s = p.second;
        if (lineid != curline) {
            if (curline == "") cout << "|  (乘坐 " << lineid << ")\n";
            else cout << "  |  (换乘 " << lineid << ")\n";
            curline = lineid;
        }
        cout << s.name << "\n";
    }
    cout << "\n总站数:   " << ans.totalstops << "\n";
    cout << "换乘次数: " << ans.totaltransfer << "\n";
    cout << "总时间:   " << ans.totaltime << "分钟\n";
}

int readint(const string& prompt)
{
    int x;
    while (true) {
        cout << prompt;
        if (!(cin >> x)) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "无效输入\n";
            continue;
        }
        cin.ignore(10000, '\n');
        return x;
    }
}

string readline(const string& prompt)
{
    cout << prompt;
    string s;
    getline(cin, s);
    return s;
}

void routequerymenu(datamanager& dm)
{
    while (true) {
        cout << "\n===== 线路查询 =====\n";
        cout << "1. 最少时间\n";
        cout << "2. 最少站点\n";
        cout << "3. 最少换乘\n";
        cout << "0. 返回\n";
        int c = readint("请选择: ");
        if (c == 0) return;
        if (c < 1 || c > 3) {
            cout << "无效选项\n";
            continue;
        }
        string from = readline("起点: ");
        string to = readline("终点: ");
        routeans ans;
        if (c == 1) {
            shortesttime st(dm);
            ans = st.find(from, to);
        }
        else if (c == 2) {
            feweststops fs(dm);
            ans = fs.find(from, to);
        }
        else if (c == 3) {
            fewesttransfers ft(dm);
            ans = ft.find(from, to);
        }
        printroute(ans);
        if (ans.routepath.empty()) {
            cout << "提示：这两个站点之间没有可达线路。\n";
        }
    }
}

void linequerymenu(datamanager& dm)
{
    string lineid = readline("请输入线路名称（如1号线):");
    auto it = dm.alllines.find(lineid);
    if (it == dm.alllines.end()) {
        cout << "未找到线路" << lineid << "\n";
        return;
    }
    const busline& line = it->second;
    cout << "\n线路：" << lineid << "\n";
    cout << "首班车：" << line.firstbus << "   末班车：" << line.lastbus << "\n";
    cout << "站点总数：" << line.stations.size() << "\n";
    cout << "站点列表：\n";
    for (int i = 0; i < line.stations.size(); i++) {
        cout << "  " << (i + 1) << ". " << line.stations[i].name;
        if (i < line.intervaltime.size()) {
            cout << "  （到下一站约 " << line.intervaltime[i] << " 分钟）";
        }
        cout << "\n";
    }
}

void modifylinemenu(datamanager& dm)
{
    string lineid = readline("请输入要修改的线路名称: ");
    auto it = dm.alllines.find(lineid);
    if (it == dm.alllines.end()) {
        cout << "未找到线路：" << lineid << "\n";
        return;
    }
    busline& line = it->second;
    cout << "\n当前线路信息\n";
    cout << "首班车：" << line.firstbus << "   末班车：" << line.lastbus << "\n";
    cout << "\n1. 修改首末班车时间\n";
    cout << "2. 删除某站点\n";
    cout << "3. 添加站点到末尾\n";
    cout << "4. 修改站间时间\n";
    cout << "0. 取消\n";
    int c = readint("请选择: ");
    if (c == 0) return;
    if (c == 1) {
        line.firstbus = readline("新的首班车时间（如 05:30）: ");
        line.lastbus = readline("新的末班车时间（如 23:00）: ");
    }
    else if (c == 2) {
        if (line.stations.empty()) {
            cout << "线路没有站点\n";
            return;
        }
        for (int i = 0; i < line.stations.size(); i++)
            cout << "  " << (i + 1) << ". " << line.stations[i].name << "\n";
        int idx = readint("要删除的站点编号: ");
        if (idx < 1 || idx > line.stations.size()) {
            cout << "编号无效\n";
            return;
        }
        line.stations.erase(line.stations.begin() + (idx - 1));
    }
    else if (c == 3) {
        string name = readline("新站点名称: ");
        int t = readint("从原最后一站到新站的时间（分钟）: ");
        line.stations.emplace_back(name);
        line.intervaltime.push_back(t);
    }
    else if (c == 4) {
        if (line.intervaltime.empty()) {
            cout << "没有站间时间可改\n";
            return;
        }
        for (int i = 0; i < line.intervaltime.size(); i++)
            cout << "  " << (i + 1) << ". " << line.stations[i].name << " -> " << line.stations[i + 1].name << " : " << line.intervaltime[i] << " 分钟\n";
        int idx = readint("要修改的段编号: ");
        if (idx < 1 || idx > line.intervaltime.size()) {
            cout << "编号无效\n";
            return;
        }
        int t = readint("新的时间（分钟）: ");
        line.intervaltime[idx - 1] = t;
    }
    else {
        cout << "无效选项\n";
        return;
    }
    dm.rebuildgraph();
    if (dm.savetofile("data.txt"))
        cout << "修改已保存到 data.txt\n";
    else
        cout << "保存失败\n";
}

void usermenu(datamanager& dm)
{
    while (true) {
        cout << "\n===== 普通用户菜单 =====\n";
        cout << "1. 快速查询（三种最优）\n";
        cout << "2. 详细查询（列多条 + 标签）\n";
        cout << "3. 车次查询\n";
        cout << "0. 返回上级\n";
        int c = readint("请选择: ");
        if (c == 0) return;
        if (c == 1) routequerymenu(dm);
        else if (c == 2) allroutesmenu(dm);
        else if (c == 3) linequerymenu(dm);
        else cout << "无效选项\n";
    }
}

void addlinemenu(datamanager& dm)
{
    string lineid = readline("新线路名称（如 1 号线）: ");
    if (dm.alllines.count(lineid)) {
        cout << "该线路已存在\n";
        return;
    }
    busline line;
    line.firstbus = readline("首班车时间（如 06:00）: ");
    line.lastbus = readline("末班车时间（如 23:00）: ");
    int n = readint("站点数量");
    if (n <= 0) {
        cout << "站点数量必须大于 0\n";
        return;
    }
    for (int i = 0; i < n; i++) {
        string name = readline("第 " + to_string(i + 1) + " 个站点名称: ");
        line.stations.emplace_back(name);
    }
    if (n > 1) {
        cout << "请输入相邻站点之间的时间（分钟），共 " << (n - 1) << " 个：\n";
        for (int i = 0; i < n - 1; i++) {
            int t = readint("第 " + to_string(i + 1) + " 段: ");
            line.intervaltime.push_back(t);
        }
    }
    dm.alllines[lineid] = line;
    dm.rebuildgraph();
    if (dm.savetofile("data.txt")) {
        cout << "新增成功，已保存到 data.txt\n";
    }
    else {
        cout << "保存失败\n";
    }
}

void adminmenu(datamanager& dm)
{
    while (true) {
        cout << "\n===== 管理员菜单 =====\n";
        cout << "1. 修改线路\n";
        cout << "2. 新增线路\n";
        cout << "0. 返回上级\n";
        int c = readint("请选择:");
        if (c == 0) return;
        if (c == 1) modifylinemenu(dm);
        else if (c == 2) addlinemenu(dm);
        else cout << "无效选项\n";
    }
}

void loginmenu(datamanager& dm)
{
    while (true) {
        cout << "\n===== 公交路线自动化选择系统 =====\n";
        cout << "1. 普通用户\n";
        cout << "2. 管理员\n";
        cout << "0. 退出系统\n";
        int c = readint("请选择: ");
        if (c == 0) {
            cout << "再见\n";
            return;
        }
        if (c == 1) {
            usermenu(dm);
        }
        else if (c == 2) {
            string pwd = readline("请输入管理员密码: ");
            if (pwd != "12345") {
                cout << "密码错误\n";
                continue;
            }
            adminmenu(dm);
        }
        else {
            cout << "无效选项\n";
        }
    }
}

// 合并一条 routeans 到 routeinfo 列表，打上兜底标记
// flag: 1=最快, 2=最少站点, 3=最少换乘
void mergeroute(vector<routeinfo>& all, const routeans& ans, int flag) {
    if (ans.routepath.empty()) return;
    // 检查是否已存在（按站点序列比较）
    for (auto& r : all) {
        if (r.path.size() != ans.routepath.size()) continue;
        bool same = true;
        for (int i = 0; i < r.path.size(); i++) {
            if (r.path[i].second.name != ans.routepath[i].second.name) {
                same = false;
                break;
            }
        }
        if (same) {
            if (flag == 1) r.isbesttime = true;
            if (flag == 2) r.isbeststops = true;
            if (flag == 3) r.isbesttrans = true;
            return;
        }
    }
    // 不存在，加进去
    routeinfo info;
    info.path = ans.routepath;
    for (auto& p : ans.routepath) {
        if (p.first != "" && (info.lines.empty() || info.lines.back() != p.first))
            info.lines.push_back(p.first);
    }
    info.totalstops = ans.totalstops;
    info.totaltransfer = ans.totaltransfer;
    info.totaltime = ans.totaltime;
    if (flag == 1) info.isbesttime = true;
    if (flag == 2) info.isbeststops = true;
    if (flag == 3) info.isbesttrans = true;
    all.push_back(info);
}

void allroutesmenu(datamanager& dm)
{
    string from = readline("起点: ");
    string to = readline("终点: ");
    routequery rq(dm);
    vector<routeinfo> all = rq.findallroutes(from, to, 3, 20);

    shortesttime st(dm);
    feweststops fs(dm);
    fewesttransfers ft(dm);
    mergeroute(all, st.find(from, to), 1);
    mergeroute(all, fs.find(from, to), 2);
    mergeroute(all, ft.find(from, to), 3);

    if (all.empty()) {
        cout << "两站之间不可达\n";
        return;
    }

    // 第一轮排序按时间
    sort(all.begin(), all.end(),
        [](const routeinfo& a, const routeinfo& b) {
            return a.totaltime < b.totaltime;
        });

    // 分离兜底和普通
    vector<routeinfo> best3, normal;
    for (auto& r : all) {
        if (r.isbesttime || r.isbeststops || r.isbesttrans)
            best3.push_back(r);
        else
            normal.push_back(r);
    }
    if (normal.size() > 6) normal.resize(6);
    all = best3;
    for (auto& r : normal) all.push_back(r);

    // 找最优值
    int bestT = INT_MAX, bestS = INT_MAX, bestX = INT_MAX;
    for (auto& r : all) {
        bestT = min(bestT, r.totaltime);
        bestS = min(bestS, r.totalstops);
        bestX = min(bestX, r.totaltransfer);
    }

    // 第二轮排序：标签数降序 + 时间升序
    sort(all.begin(), all.end(),
        [&](const routeinfo& a, const routeinfo& b) {
            int ca = 0, cb = 0;
            if (a.totaltime == bestT)     ca++;
            if (a.totalstops == bestS)    ca++;
            if (a.totaltransfer == bestX) ca++;
            if (b.totaltime == bestT)     cb++;
            if (b.totalstops == bestS)    cb++;
            if (b.totaltransfer == bestX) cb++;
            if (ca != cb) return ca > cb;
            if (a.totaltime != b.totaltime) return a.totaltime < b.totaltime;
            return a.totalstops < b.totalstops;
        });

    cout << "\n共找到 " << all.size() << " 条可达路线：\n";
    for (int i = 0; i < all.size(); i++) {
        const routeinfo& r = all[i];
        cout << "\n[" << i + 1 << "] ";
        for (int j = 0; j < r.lines.size(); j++) {
            cout << r.lines[j];
            if (j + 1 < r.lines.size()) cout << " -> ";
        }
        cout << "  【";
        bool first = true;
        if (r.totaltime == bestT) {
            cout << (first ? "" : " ") << "最快";
            first = false;
        }
        if (r.totalstops == bestS) {
            cout << (first ? "" : " ") << "最少站点";
            first = false;
        }
        if (r.totaltransfer == bestX) {
            cout << (first ? "" : " ") << "最少换乘";
            first = false;
        }
        cout << "】\n";
        cout << "    总站数 " << r.totalstops
             << "，换乘 " << r.totaltransfer << " 次"
             << "，约 " << r.totaltime << " 分钟\n";
        cout << "    ";
        for (int j = 0; j < r.path.size(); j++) {
            cout << r.path[j].second.name;
            if (j + 1 < r.path.size()) cout << " -> ";
        }
        cout << "\n";
    }
}
