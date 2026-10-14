# 使用一个包含 GCC 编译器的官方镜像作为构建环境
FROM gcc:13

# 设置容器内的工作目录
WORKDIR /app

# 将当前目录下的所有文件复制到容器的 /app 目录
COPY . .

# 编译 C++ 程序，链接必要的系统库
# -lws2_32 是 Windows 下的库，在 Linux 环境中不需要，需移除
RUN g++ -o bus_server main.cpp menu.cpp routequery.cpp datamanager.cpp shortesttime.cpp feweststops.cpp fewesttransfers.cpp -std=c++11 -lpthread
# 声明容器在运行时监听的端口，与代码中绑定的一致
# 声明容器在运行时监听的端口，与代码中绑定的一致
EXPOSE 8080

# 容器启动时执行的命令
CMD ["./bus_server"]
