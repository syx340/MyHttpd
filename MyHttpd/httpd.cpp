# include <iostream>
using namespace std;
#include<sys/types.h>
#include<sys/stat.h>
# include <WinSock2.h>  // winsock2.h���׽��ֽӿ�
#pragma comment(lib,"WS2_32.lib") // ws2_32.lib���׽���ʵ��
#define PRINTF(str) printf("[%s -%d]"#str"=%s\r\n",__func__,__LINE__,str);

void error_die(const char* str)
{
	perror(str);  // �ȴ�ӡ��������ַ�
	exit(1);
}
// ʵ������ĳ�ʼ��
// ����ֵ���׽��֣��������˵��׽��֣�
// �˿�
// ������port ��ʾ�˿�
//				���*port��ֵΪ0����ô�Զ�����һ�����õĶ˿�
//				Ϊ��
int startup(unsigned short* port)
{
	// 1.����ͨ�ų�ʼ����Linux ����Ҫ��ʼ����	
	WSADATA wsaData;
	int ret = WSAStartup(MAKEWORD(1, 1),  // 1.1�汾��Э��
		&wsaData);
	if (ret) {
		error_die("WSAStartup");
	}
	// 2.�����׽���
	int server_socket = socket(PF_INET, // �׽�������
		SOCK_STREAM,  // ������
		IPPROTO_TCP); // ͨ��Э��
	if (server_socket == -1)
	{
		error_die("socket");
	}
	//  ���ö˿ڵĿɸ���
	int opt = 1;
	ret = setsockopt(server_socket, SOL_SOCKET, //�׽��ֲ��������  
		SO_REUSEADDR,  // �˿ڿ��ظ�ʹ�õ�����
		(const char*)&opt, sizeof(opt)); // ����ֵΪ1
	if (ret == -1)
	{
		error_die("setsockoopt");
	}
	//���÷������˵������ַ
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr)); // ��sockaddr_in��Ա��������Ϊ0
	server_addr.sin_family = AF_INET;  // �׽������� ��PF_INET��
	server_addr.sin_port = htons(*port);//��С��ת��
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//3.���׽���
	if (bind(server_socket, (struct sockaddr*)&server_addr,
		sizeof(server_addr)) < 0)
	{
		error_die("bind");
	}
	//4.��̬����˿�
	if (*port == 0) {
		int namelen = sizeof(server_addr);
		if (getsockname(server_socket, (struct sockaddr*)&server_addr, &namelen) == -1) {
			error_die("getsockname");
		}
		*port = ntohs(server_addr.sin_port);
	}
	//������������
	if (listen(server_socket, 5) < 0)
	{
		error_die("listen");     // ���粻�ȶ��������������μ�鷵��ֵ�Ǻ�ϰ��
	}
	return server_socket;
}
//��ָ���Ŀͻ����׽���sock����ȡһ�����ݣ����浽buff��
//����ʵ�ʶ�ȡ�����ֽ�
int get_line(int sock, char* buff, int size)
{
	char c = 0; //'\0'
	int i = 0;

	//\r\n(���з��ͻس���)
	while (i<size-1&&c != '\n')
	{
		int n = recv(sock, &c, 1, 0);//��һ���ַ�
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
	buff[i] = 0; //'\0' �����ַ������Ļس���
	return (i);
}

void unimplement(int client)
{
	//��ָ�����׽��֣�����һ����ʾ��û��ʵ�ֵĴ���ҳ��
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
	//����404��Ӧ
	char buff[1024];

	sprintf(buff, "HTTP/1.0 404 NOT FOUND\r\n");
	send(client, buff, strlen(buff), 0);

	sprintf(buff, "Server: SxyHttpd/0.1\r\n");
	send(client, buff, strlen(buff), 0);

	sprintf(buff, "Content-type: text/html\n");
	send(client, buff, strlen(buff), 0);

	sprintf(buff, "\r\n");
	send(client, buff, strlen(buff), 0);

	//����404��ҳ����
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
	//������Ӧ����ͷ��Ϣ
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

void cat(int client, FILE* resource)//�ļ�ʵ������
{
	// tinyhttp ��һ�ζ�һ���ֽڣ�Ȼ���ٰ�����ֽڷ��͸������
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
	printf("һ������[%d]�ֽڸ������\n", count);
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

	//���������ݰ���ʣ�������ж���
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
		//��ʽ������Դ�������
	
		headers(client,getHeadType(fileName));

		//�����������Դ��Ϣ
		cat(client, resource);
		printf("��Դ�������!\n");
	}
	fclose(resource);
}
// �����̺߳���
DWORD WINAPI accept_request(LPVOID arg) {
	char buff[1024];// 1k
	int client = (SOCKET)arg;
	//��ȡһ������
	// GET / HTTP / 1.1\n
	int numchars = get_line(client, buff, sizeof(buff));
	PRINTF(buff);// [accept_request-119]buff="GET........"
	// ������Դ
	char method[255];
	int j = 0, i = 0;
	while (!isspace(buff[j])&&i<sizeof(method) - 1)
	{
		method[i++] = buff[j++];
	}
	method[i] = 0;// '\0'
	PRINTF(method);
	//�������ķ��������������Ƿ�֧��
	if (stricmp(method, "GET") && stricmp(method, "POST"))
	{
		//�����������һ��������ʾ����
		unimplement(client);
		return 0;
	}	
	//������Դ�ļ���·��
	char url[255];//����������Դ��������
	i = 0;
	//������Դ·��ǰ��Ŀո�
	while (isspace(buff[j]) && j < sizeof(buff)) j++;
	while (!isspace(buff[j]) && i < sizeof(url) - 1 && j < sizeof(buff))
	{
		url[i++] = buff[j++];
	}
	url[i] = 0;
	PRINTF(url);
	//��ȡ��������Դ������·��
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
		//�������ʣ�����ݶ�ȡ���
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
	cout<<"httpd�����Ѿ����������ڼ��� "<<port<<" �˿�..."<<endl;

	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	//����ʽ�ȴ��ͻ��˷�����ʣ�ͨ�ýӿڣ�linuxҲ���ã�
	while (1)
	{
		int client_sock = accept(server_sock, (struct sockaddr*) & client_addr,&client_addr_len );
		if (client_sock == -1)
		{
			error_die("accept");
		}

		//����һ���µ��߳�
		//����(���԰�������߳�)
		//CreateThread�ķ���ֵ���̵߳ľ��,ʧ�ܵĻ��ͷ���NULL
		DWORD threadId = 0;
		CreateThread(0, 0, accept_request, //�̺߳���,�����������Ϊ��void*��
			(void*)client_sock, 0, &threadId);
	}
	closesocket(server_sock);
	system("pause");
	return 0;
}
