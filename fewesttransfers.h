#pragma once
#include <string>
#include "datamanager.h"
// 接下来的最小换乘数，一样的，算法
class fewesttransfers {
public:
	fewesttransfers(datamanager& dm) :dm(dm) {}//构造时，把传进来的 datamanager&绑定到成员变量 dm 上。
	routeans find(const string& from, const string& to);//调用：routeans ans=st.find(,);
private:
	datamanager& dm;
	int transfercost = 5;
};
//更简单了，因为对比shortesttime算法，只需要改变代价的次序即可。