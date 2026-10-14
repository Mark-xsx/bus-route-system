#pragma once
#include <string>
#include "datamanager.h"
#include "routequery.h"     // 新增：用到 routeinfo
#include "json.hpp"

using json = nlohmann::json;

// ========== Web 业务处理函数 ==========
json handleLogin(const json& req);
json handleRouteQuery(const json& req, datamanager& dm);
json handleLineQuery(const json& req, datamanager& dm);
json handleAddLine(const json& req, datamanager& dm);
json handleModifyLine(const json& req, datamanager& dm);
json handleGetLines(datamanager& dm);
json handleAllRoutes(const json& req, datamanager& dm);   // 新增：所有可达路线
