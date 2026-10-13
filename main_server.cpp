#include "httplib.h"
#include "json.hpp"
#include "datamanager.h"
#include "shortesttime.h"
#include "feweststops.h"
#include "fewesttransfers.h"

#include <iostream>
#include <string>

using namespace std;
using json = nlohmann::json;

// 全局数据管理器
datamanager dm;

// 把 routeans 转成 JSON
json routeToJson(const routeans& ans) {
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

int main() {
    if (!dm.loadfromfile("data.txt")) {
        cout << "加载 data.txt 失败\n";
        return 1;
    }

    httplib::Server svr;

    // 1. 登录
    svr.Post("/api/login", [](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body);
        string role = body.value("role", "");
        string pwd  = body.value("password", "");

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
        res.set_content(out.dump(), "application/json");
    });

    // 2. 线路查询：策略 1/2/3
    svr.Post("/api/route_query", [](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body);
        string from = body.value("from", "");
        string to   = body.value("to", "");
        int strategy = body.value("strategy", 1);

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
            res.set_content(R"({"success":false,"message":"无效策略"})", "application/json");
            return;
        }
        res.set_content(routeToJson(ans).dump(), "application/json");
    });

    // 3. 车次查询
    svr.Post("/api/line_query", [](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body);
        string lineid = body.value("lineid", "");

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
        res.set_content(out.dump(), "application/json");
    });

    // 4. 管理员：新增线路
    svr.Post("/api/admin/add_line", [](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body);
        string lineid = body.value("lineid", "");
        if (dm.alllines.count(lineid)) {
            res.set_content(R"({"success":false,"message":"线路已存在"})", "application/json");
            return;
        }

        busline line;
        line.firstbus = body.value("firstbus", "");
        line.lastbus  = body.value("lastbus", "");

        for (auto& s : body["stations"]) {
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

        json out;
        out["success"] = ok;
        out["message"] = ok ? "新增成功" : "保存失败";
        res.set_content(out.dump(), "application/json");
    });

    // 5. 管理员：修改线路（整条替换）
    svr.Post("/api/admin/modify_line", [](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body);
        string lineid = body.value("lineid", "");
        auto it = dm.alllines.find(lineid);
        if (it == dm.alllines.end()) {
            res.set_content(R"({"success":false,"message":"线路不存在"})", "application/json");
            return;
        }

        busline& line = it->second;
        line.firstbus = body.value("firstbus", "");
        line.lastbus  = body.value("lastbus", "");
        line.stations.clear();
        line.intervaltime.clear();

        for (auto& s : body["stations"]) {
            string name = s.value("name", "");
            int interval = s.value("interval", 0);
            line.stations.emplace_back(name);
            line.intervaltime.push_back(interval);
        }
        if (!line.intervaltime.empty()) line.intervaltime.pop_back();

        dm.rebuildgraph();
        bool ok = dm.savetofile("data.txt");

        json out;
        out["success"] = ok;
        out["message"] = ok ? "修改成功" : "保存失败";
        res.set_content(out.dump(), "application/json");
    });

    // 6. 获取所有线路名（管理员下拉框用）
    svr.Get("/api/lines", [](const httplib::Request&, httplib::Response& res) {
        json arr = json::array();
        for (auto& p : dm.alllines) arr.push_back(p.first);
        res.set_content(arr.dump(), "application/json");
    });

    // 托管前端静态文件
    svr.set_mount_point("/", "./web");

    cout << "服务已启动：http://127.0.0.1:8080\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}