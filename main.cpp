#include <stdio.h>    
#include <stdlib.h>   //atoi头文件
#include <unistd.h>   //chdir头文件
#include "TcpServer.h"


int main(int argc, char* argv[]) {
#if 0
    if(argc < 3) {
        printf("please input: ./a.out port path\n");
        return -1;
    } 
    unsigned short port = atoi(argv[1]);
    //切换服务器的工作路径为客户端请求的资源路径
    chdir(argv[2]);
#else
    unsigned short port = 10010;      //浏览器访问: 47.121.116.225:10010
    chdir("/projects/sources");
#endif
    //创建服务器实例
    TcpServer * server = new TcpServer(port, 4);
    server->run();
    return 0;
}