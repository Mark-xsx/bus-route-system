#include "menu.h"
#include "shortesttime.h"
#include "feweststops.h"
#include "fewesttransfers.h"

using namespace std;

// ==================== 登录 ====================
// 参数 req：包含 role 和 password
// 返回：包含 success 和 message 的 JSON
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

// ==================== 线路查询 ====================
// 参数 req：包含 from、to、strategy（1=最少时间，2=最少站点，3=最少换乘）
// 返回：包含路线步骤和统计信息的 JSON
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
        step["line"] = p.first;           // 线路名
        step["station"] = p.second.name;  // 站点名
        steps.push_back(step);
    }
    j["steps"] = steps;
    return j;
}

// ==================== 车次查询 ====================
// 参数 req：包含 lineid
// 返回：包含线路名、首末班车、所有站点的 JSON
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

// ==================== 管理员：新增线路 ====================
// 参数 req：包含 lineid、firstbus、lastbus、stations（数组，每项含 name 和 interval）
// 返回：包含 success 和 message 的 JSON
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
    // 最后一个 interval 无意义，去掉
    if (!line.intervaltime.empty()) line.intervaltime.pop_back();

    dm.alllines[lineid] = line;
    dm.rebuildgraph();
    bool ok = dm.savetofile("data.txt");

    out["success"] = ok;
    out["message"] = ok ? "新增成功" : "保存失败";
    return out;
}

// ==================== 管理员：修改线路 ====================
// 参数 req：包含 lineid、firstbus、lastbus、stations
// 返回：包含 success 和 message 的 JSON
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

// ==================== 获取所有线路名 ====================
// 返回：包含所有线路名的 JSON 数组
json handleGetLines(datamanager& dm) {
    json arr = json::array();
    for (auto& p : dm.alllines) arr.push_back(p.first);
    return arr;
}