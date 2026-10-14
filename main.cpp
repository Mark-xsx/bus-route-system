#include "httplib.h"
#include "json.hpp"
#include "datamanager.h"
#include "menu.h"
#include <iostream>
#include <cstdlib>

using namespace std;
using json = nlohmann::json;

int main() {
    datamanager dm;
    if (!dm.loadfromfile("data.txt")) {
        cout << "加载 data.txt 失败\n";
        return 1;
    }
    cout << "共加载 " << dm.alllines.size() << " 条线路\n";
    cout << "共收集 " << dm.allstations.size() << " 个站点\n";
    cout << "邻接图节点数 " << dm.adjgraph.size() << "\n";

    httplib::Server svr;

    // 1. 登录
    svr.Post("/api/login", [&](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body);
        json out = handleLogin(body);
        res.set_content(out.dump(), "application/json");
    });

    // 2. 线路查询
    svr.Post("/api/route_query", [&](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body);
        json out = handleRouteQuery(body, dm);
        res.set_content(out.dump(), "application/json");
    });

    // 3. 车次查询
    svr.Post("/api/line_query", [&](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body);
        json out = handleLineQuery(body, dm);
        res.set_content(out.dump(), "application/json");
    });

    // 4. 管理员新增线路
    svr.Post("/api/admin/add_line", [&](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body);
        json out = handleAddLine(body, dm);
        res.set_content(out.dump(), "application/json");
    });

    // 5. 管理员修改线路
    svr.Post("/api/admin/modify_line", [&](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body);
        json out = handleModifyLine(body, dm);
        res.set_content(out.dump(), "application/json");
    });

    // 6. 获取所有线路名
    svr.Get("/api/lines", [&](const httplib::Request&, httplib::Response& res) {
        json out = handleGetLines(dm);
        res.set_content(out.dump(), "application/json");
    });
     // 7. 所有可达路线
svr.Post("/api/all_routes", [&](const httplib::Request& req, httplib::Response& res) {
    auto body = json::parse(req.body);
    json out = handleAllRoutes(body, dm);
    res.set_content(out.dump(), "application/json");
});

    // 托管前端静态文件
    svr.set_mount_point("/", "./web");

    const char* port_env = std::getenv("PORT");
    int port = port_env ? std::stoi(port_env) : 8080;

    cout << "服务已启动，监听端口: " << port << endl;
    svr.listen("0.0.0.0", port);
    return 0;
}
