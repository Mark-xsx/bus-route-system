#pragma once
#include<string>
#include"datamanager.h"
class shortesttime {
public:
	shortesttime(datamanager& dm) :dm(dm) {}//构造时，把传进来的 datamanager&绑定到成员变量 dm 上。
	routeans find(const string& from, const string& to);//调用：routeans ans=st.find(,);
private:
	datamanager& dm;
	int transfercost = 5;
};