# include <iostream>
using namespace std;
#include<sys/types.h>
#include<sys/stat.h>
# include <WinSock2.h>  // winsock2.h是套接字接口
#pragma comment(lib,"WS2_32.lib") // ws2_32.lib是套接字实现
#define PRINTF(str) printf("[%s -%d]"#str"=%s\r\n",__func__,__LINE__,str);

void error_die(const char* str)
{
	perror(str);  // 先打印出错误的字符
	exit(1);
}
// 实现网络的初始化
// 返回值：套接字（服务器端的套接字）
// 端口
// 参数：port 表示端口
//				如果*port的值为0，那么自动分配一个可用的端口
//				为了
int startup(unsigned short* port)
{
	// 1.网络通信初始化（Linux 不需要初始化）	
	WSADATA wsaData;
	int ret = WSAStartup(MAKEWORD(1, 1),  // 1.1版本的协议
		&wsaData);
	if (ret) {
		error_die("WSAStartup");
	}
	// 2.创建套接字
	int server_socket = socket(PF_INET, // 套接字类型
		SOCK_STREAM,  // 数据流
		IPPROTO_TCP); // 通信协议
	if (server_socket == -1)
	{
		error_die("socket");
	}
	//  设置端口的可复用
	int opt = 1;
	ret = setsockopt(server_socket, SOL_SOCKET, //套接字层面的属性  
		SO_REUSEADDR,  // 端口可重复使用的属性
		(const char*)&opt, sizeof(opt)); // 参数值为1
	if (ret == -1)
	{
		error_die("setsockoopt");
	}
	//配置服务器端的网络地址
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr)); // 将sockaddr_in成员属性设置为0
	server_addr.sin_family = AF_INET;  // 套接字类型 （PF_INET）
	server_addr.sin_port = htons(*port);//大小端转换
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//3.绑定套接字
	if (bind(server_socket, (struct sockaddr*)&server_addr,
		sizeof(server_addr)) < 0)
	{
		error_die("bind");
	}
	//4.动态分配端口
	if (*port == 0) {
		int namelen = sizeof(server_addr);
		if (getsockname(server_socket, (struct sockaddr*)&server_addr, &namelen) == -1) {
			error_die("getsockname");
		}
		*port = ntohs(server_addr.sin_port);
	}
	//创建监听队列
	if (listen(server_socket, 5) < 0)
	{
		error_die("listen");     // 网络不稳定，多次启动，多次检查返回值是好习惯
	}
	return server_socket;
}
//从指定的客户端套接字sock，读取一行数据，保存到buff中
//返回实际读取到的字节
int get_line(int sock, char* buff, int size)
{
	char c = 0; //'\0'
	int i = 0;

	//\r\n(换行符和回车符)
	while (i<size-1&&c != '\n')
	{
		int n = recv(sock, &c, 1, 0);//读一个字符
		if (n > 0)
		{
			if (c == '\r')
			{
				n = recv(sock, &c, 1, MSG_PEEK);
				if (n > 0 && c == '\n')
				{
					recv(sock, &c, 1, 0);
				}
				else {
					c = '\n';
				}
			}
			buff[i++] = c;
		}
		else
		{
			c = '\n';
		}
	}
	buff[i] = 0; //'\0' 处理字符串最后的回车符
	return (i);
}

void unimplement(int client)
{
	//向指定的套接字，发送一个提示还没有实现的错误页面
	char buf[1024];

	sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Server: SxyHttpd/0.1\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</TITLE></HEAD>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);

}

void not_found(int client)
{
	//发送404响应
	char buff[1024];

	sprintf(buff, "HTTP/1.0 404 NOT FOUND\r\n");
	send(client, buff, strlen(buff), 0);

	sprintf(buff, "Server: SxyHttpd/0.1\r\n");
	send(client, buff, strlen(buff), 0);

	sprintf(buff, "Content-type: text/html\n");
	send(client, buff, strlen(buff), 0);

	sprintf(buff, "\r\n");
	send(client, buff, strlen(buff), 0);

	//发送404网页内容
	sprintf(buff, "<HTML><TITLE>Not Found</TITLE>\r\n");
	send(client, buff, strlen(buff), 0);
	sprintf(buff, "<BODY><P>The server could not fulfill\r\n");
	send(client, buff, strlen(buff), 0);
	sprintf(buff, "your request because the resource specified\r\n");
	send(client, buff, strlen(buff), 0);
	sprintf(buff, "is unavailable or nonexistent.\r\n");
	send(client, buff, strlen(buff), 0);
	sprintf(buff, "</BODY></HTML>\r\n");
	send(client, buff, strlen(buff), 0);
}

void headers(int client,const char* type)
{
	//发送响应包的头信息
	char buff[1024];

	strcpy(buff, "HTTP/1.0 200 OK\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "Server: SxyHttpd/0.1\r\n");
	send(client, buff, strlen(buff), 0);


	char buf[1024];
	sprintf(buf, "Content-type: %s\r\n",type);
	send(client, buf, strlen(buf), 0);

	strcpy(buff, "\r\n");
	send(client, buff, strlen(buff), 0);
}

void cat(int client, FILE* resource)//文件实际内容
{
	// tinyhttp 是一次读一个字节，然后再把这个字节发送给浏览器
	char buff[4096];
	int count = 0;

	while (1)
	{
		int ret = fread(buff, sizeof(char), sizeof(buff), resource);
		if (ret <= 0)
		{
			break;
		}
		send(client, buff, ret, 0);
		count += ret;
	}
	printf("一共发送[%d]字节给浏览器\n", count);
}

const char* getHeadType(const char* fileName) {
	const char* ret = "text/html";
	const char* p = strrchr(fileName, '.');
	if (!p) return ret;

	p++;
	if (!strcmp(p, "css")) ret = "text/css";
	else if (!strcmp(p, "jpg")) ret = "img/jpeg";
	else if (!strcmp(p, "png")) ret = "img/png";
	else if (!strcmp(p, "js")) ret = "application/x-javascript";

	return ret;
}
void server_file(int client, const char* fileName)
{
	int numchars = 1;
	char buff[1024];

	//把请求数据包的剩余数据行读完
	while (numchars > 0 && strcmp(buff, "\n"))
	{
		numchars = get_line(client, buff, sizeof(buff));
		buff[numchars] = 0;
		PRINTF(buff);
	}

	FILE* resource = NULL;
	if (strcmp(fileName, "htsouces/index.html") == 0)
	{
		resource = fopen(fileName, "r");
	}
	else {
		resource = fopen(fileName, "rb");
	}
	if (resource == NULL) {
		not_found(client);
	}
	else {
		//正式发送资源给浏览器
	
		headers(client,getHeadType(fileName));

		//发送请求的资源信息
		cat(client, resource);
		printf("资源发送完毕!\n");
	}
	fclose(resource);
}
// 创建线程函数
DWORD WINAPI accept_request(LPVOID arg) {
	char buff[1024];// 1k
	int client = (SOCKET)arg;
	//读取一行数据
	// GET / HTTP / 1.1\n
	int numchars = get_line(client, buff, sizeof(buff));
	PRINTF(buff);// [accept_request-119]buff="GET........"
	// 解析资源
	char method[255];
	int j = 0, i = 0;
	while (!isspace(buff[j])&&i<sizeof(method) - 1)
	{
		method[i++] = buff[j++];
	}
	method[i] = 0;// '\0'
	PRINTF(method);
	//检查请求的方法，本服务器是否支持
	if (stricmp(method, "GET") && stricmp(method, "POST"))
	{
		//向浏览器返回一个错误提示界面
		unimplement(client);
		return 0;
	}	
	//解析资源文件的路径
	char url[255];//存放请求的资源的完整性
	i = 0;
	//跳过资源路径前面的空格
	while (isspace(buff[j]) && j < sizeof(buff)) j++;
	while (!isspace(buff[j]) && i < sizeof(url) - 1 && j < sizeof(buff))
	{
		url[i++] = buff[j++];
	}
	url[i] = 0;
	PRINTF(url);
	//获取服务器资源的完整路径
	//127.0.0.1
	//htsouces/img

	char path[512] = ""; 
	sprintf(path, "htsouces%s", url);
	if (path[strlen(path) - 1] == '/')
	{
		strcat(path, "index.html");
	}
	PRINTF(path);

	struct stat status;
	if (stat(path, &status) == -1)
	{
		//请求包的剩余数据读取完毕
		while (numchars > 0 && strcmp(buff, "\n"))
		{
			numchars = get_line(client, buff, sizeof(buff));
		}
		not_found(client);
	}
	else {
		if ((status.st_mode & S_IFMT) == S_IFDIR) {
			strcat(path, "/index.html");
		}
		server_file(client, path);

	}

	closesocket(client);
	return 0;
}

int main(void)
{
	unsigned short port = 80; 
	int server_sock = startup(&port);
	cout<<"httpd服务已经启动，正在监听 "<<port<<" 端口..."<<endl;

	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	//阻塞式等待客户端发起访问（通用接口，linux也适用）
	while (1)
	{
		int client_sock = accept(server_sock, (struct sockaddr*) & client_addr,&client_addr_len );
		if (client_sock == -1)
		{
			error_die("accept");
		}

		//创建一个新的线程
		//进程(可以包含多个线程)
		//CreateThread的返回值是线程的句柄,失败的话就返回NULL
		DWORD threadId = 0;
		CreateThread(0, 0, accept_request, //线程函数,传入参数类型为（void*）
			(void*)client_sock, 0, &threadId);
	}
	closesocket(server_sock);
	system("pause");
	return 0;
}
