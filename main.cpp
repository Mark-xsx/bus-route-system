#include "datamanager.h"
#include "shortesttime.h"
#include "feweststops.h"
#include "fewesttransfers.h"
#include <limits>

/**
 * 打印路线结果
 * @param ans 路线结果结构体
 */
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

/**
 * 读取一个整数，带输入校验
 * @param prompt 提示语
 * @return 用户输入的整数
 */
int readInt(const string& prompt)
{
    int x;
    while (true) {
        cout << prompt;
        if (cin >> x) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return x;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "无效输入，请重新输入。\n";
    }
}

/**
 * 读取一行字符串
 * @param prompt 提示语
 * @return 用户输入的字符串
 */
string readLine(const string& prompt)
{
    cout << prompt;
    string s;
    getline(cin, s);
    return s;
}

/**
 * 线路查询：输入起点终点，选择策略，输出结果
 * @param dm 数据管理器引用
 */
void routeQueryMenu(datamanager& dm)
{
    while (true) {
        cout << "\n===== 线路查询 =====\n";
        cout << "1. 最少时间\n";
        cout << "2. 最少站点\n";
        cout << "3. 最少换乘\n";
        cout << "0. 返回\n";

        int c = readInt("请选择: ");
        if (c == 0) return;
        if (c < 1 || c > 3) {
            cout << "无效选项\n";
            continue;
        }

        string from = readLine("起点: ");
        string to = readLine("终点: ");

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

/**
 * 车次查询：输入线路名，显示该线路所有站点、首末班车、站间时间
 * @param dm 数据管理器引用
 */
void lineQueryMenu(datamanager& dm)
{
    string lineid = readLine("请输入线路名称（如 1 号线）: ");
    auto it = dm.alllines.find(lineid);
    if (it == dm.alllines.end()) {
        cout << "未找到线路：" << lineid << "\n";
        return;
    }
    const busline& line = it->second;
    cout << "\n线路：" << lineid << "\n";
    cout << "首班车：" << line.firstbus << "   末班车：" << line.lastbus << "\n";
    cout << "站点总数：" << line.stations.size() << "\n";
    cout << "站点列表：\n";
    for (size_t i = 0; i < line.stations.size(); ++i) {
        cout << "  " << (i + 1) << ". " << line.stations[i].name;
        if (i < line.intervaltime.size()) {
            cout << "  （到下一站约 " << line.intervaltime[i] << " 分钟）";
        }
        cout << "\n";
    }
}

/**
 * 普通用户菜单
 * @param dm 数据管理器引用
 */
void userMenu(datamanager& dm)
{
    while (true) {
        cout << "\n===== 普通用户菜单 =====\n";
        cout << "1. 线路查询\n";
        cout << "2. 车次查询\n";
        cout << "0. 返回上级\n";

        int c = readInt("请选择: ");
        if (c == 0) return;
        if (c == 1) routeQueryMenu(dm);
        else if (c == 2) lineQueryMenu(dm);
        else cout << "无效选项\n";
    }
}

/**
 * 修改线路
 * @param dm 数据管理器引用
 */
void modifyLineMenu(datamanager& dm)
{
    string lineid = readLine("请输入要修改的线路名称: ");
    auto it = dm.alllines.find(lineid);
    if (it == dm.alllines.end()) {
        cout << "未找到线路：" << lineid << "\n";
        return;
    }

    busline& line = it->second;
    cout << "\n当前线路信息：\n";
    cout << "首班车：" << line.firstbus << "   末班车：" << line.lastbus << "\n";

    cout << "\n1. 修改首末班车时间\n";
    cout << "2. 删除某站点\n";
    cout << "3. 添加站点到末尾\n";
    cout << "4. 修改站间时间\n";
    cout << "0. 取消\n";
    int c = readInt("请选择: ");
    if (c == 0) return;

    if (c == 1) {
        line.firstbus = readLine("新的首班车时间（如 05:30）: ");
        line.lastbus = readLine("新的末班车时间（如 23:00）: ");
    }
    else if (c == 2) {
        if (line.stations.empty()) {
            cout << "线路没有站点\n";
            return;
        }
        for (size_t i = 0; i < line.stations.size(); ++i)
            cout << "  " << (i + 1) << ". " << line.stations[i].name << "\n";
        int idx = readInt("要删除的站点编号: ");
        if (idx < 1 || idx >(int)line.stations.size()) {
            cout << "编号无效\n";
            return;
        }
        line.stations.erase(line.stations.begin() + (idx - 1));
        if (!line.intervaltime.empty()) {
            if (idx == 1) line.intervaltime.erase(line.intervaltime.begin());
            else line.intervaltime.erase(line.intervaltime.begin() + (idx - 2));
        }
    }
    else if (c == 3) {
        string name = readLine("新站点名称: ");
        int t = readInt("从原最后一站到新站的时间（分钟）: ");
        line.stations.emplace_back(name);
        line.intervaltime.push_back(t);
    }
    else if (c == 4) {
        if (line.intervaltime.empty()) {
            cout << "没有站间时间可改\n";
            return;
        }
        for (size_t i = 0; i < line.intervaltime.size(); ++i)
            cout << "  " << (i + 1) << ". " << line.stations[i].name
            << " -> " << line.stations[i + 1].name
            << " : " << line.intervaltime[i] << " 分钟\n";
        int idx = readInt("要修改的段编号: ");
        if (idx < 1 || idx >(int)line.intervaltime.size()) {
            cout << "编号无效\n";
            return;
        }
        int t = readInt("新的时间（分钟）: ");
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

/**
 * 新增线路
 * @param dm 数据管理器引用
 */
void addLineMenu(datamanager& dm)
{
    string lineid = readLine("新线路名称（如 11 号线）: ");
    if (dm.alllines.count(lineid)) {
        cout << "该线路已存在\n";
        return;
    }

    busline line;
    line.firstbus = readLine("首班车时间（如 06:00）: ");
    line.lastbus = readLine("末班车时间（如 23:00）: ");

    int n = readInt("站点数量: ");
    if (n <= 0) {
        cout << "站点数量必须大于 0\n";
        return;
    }
    for (int i = 0; i < n; ++i) {
        string name = readLine("第 " + to_string(i + 1) + " 个站点名称: ");
        line.stations.emplace_back(name);
    }

    if (n > 1) {
        cout << "请输入相邻站点之间的时间（分钟），共 " << (n - 1) << " 个：\n";
        for (int i = 0; i < n - 1; ++i) {
            int t = readInt("第 " + to_string(i + 1) + " 段: ");
            line.intervaltime.push_back(t);
        }
    }

    dm.alllines[lineid] = line;
    dm.rebuildgraph();
    if (dm.savetofile("data.txt"))
        cout << "新增成功，已保存到 data.txt\n";
    else
        cout << "保存失败\n";
}

/**
 * 管理员菜单
 * @param dm 数据管理器引用
 */
void adminMenu(datamanager& dm)
{
    while (true) {
        cout << "\n===== 管理员菜单 =====\n";
        cout << "1. 修改线路\n";
        cout << "2. 新增线路\n";
        cout << "0. 返回上级\n";

        int c = readInt("请选择: ");
        if (c == 0) return;
        if (c == 1) modifyLineMenu(dm);
        else if (c == 2) addLineMenu(dm);
        else cout << "无效选项\n";
    }
}

/**
 * 登录菜单：选择普通用户或管理员
 * @param dm 数据管理器引用
 */
void loginMenu(datamanager& dm)
{
    while (true) {
        cout << "\n===== 公交路线自动化选择系统 =====\n";
        cout << "1. 普通用户\n";
        cout << "2. 管理员\n";
        cout << "0. 退出系统\n";

        int c = readInt("请选择: ");
        if (c == 0) {
            cout << "再见\n";
            return;
        }
        if (c == 1) {
            userMenu(dm);
        }
        else if (c == 2) {
            string pwd = readLine("请输入管理员密码: ");
            if (pwd != "12345") {
                cout << "密码错误\n";
                continue;
            }
            adminMenu(dm);
        }
        else {
            cout << "无效选项\n";
        }
    }
}

int main()
{
    datamanager dm;
    if (!dm.loadfromfile("data.txt")) {
        cout << "加载失败！检查文件格式\n";
        return 1;
    }
    cout << "共加载" << dm.alllines.size() << "条线路\n";
    cout << "共收集" << dm.allstations.size() << "个站点\n";
    cout << "邻接图节点数" << dm.adjgraph.size() << "\n\n";

    loginMenu(dm);
    return 0;
}