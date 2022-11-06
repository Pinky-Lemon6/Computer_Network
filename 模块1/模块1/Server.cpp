#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#include "winsock2.h"
#include <stdio.h>
#include <iostream>
using namespace std;

#pragma comment(lib,"ws2_32.lib")


//定位文件名后缀
char* file_type_addr(char* argue) {
	char* temp;
	if ((temp = strchr(argue, '.')) != NULL) {
		return temp + 1;
	}
	return (char*)"";
}

//构造并发送响应报文头部
void send_head(char* argue, SOCKET s, char* filename) {
	/*解析文件*/
	char* filetype = file_type_addr(argue);
	char* content_type = (char*)"text/plain";
	char* body_length = (char*)"Content-Length：";

	if (!strcmp(filetype, "html")) {
		content_type = (char*)"text/html";
	}
	else if (!strcmp(filetype, "gif")) {
		content_type = (char*)"image/gif";
	}
	else if (!strcmp(filetype, "jpg")) {
		content_type = (char*)"image/jpg";
	}
	else if (!strcmp(filetype, "png")) {
		content_type = (char*)"iamge/png";
	}
	else {
		content_type = NULL;
	}


	/*构造响应报文头部*/
	char* head = (char*)"HTTP/1.1 200 OK\r\n";
	char* not_found = (char*)"HTTP/1.1 404 NOT FOUND\r\n";
	char temp[50] = "Content-type：";


	/*获取文件并发送*/
	FILE* pfile;
	errno_t error = fopen_s(&pfile, filename, "rb");
	if (error) {
		send(s, not_found, strlen(not_found), 0);
	}
	else if (send(s, head, strlen(head), 0) == -1) {
		cout << "Sending Error!" << endl;
		return;
	}
	if (content_type) {
		strcat_s(temp, strlen(temp) + strlen(content_type) + 1, content_type);
		strcat_s(temp, strlen(temp) + 5, "\r\n");

		if (send(s, temp, strlen(temp), 0) == -1) {
			cout << "Sending Error!" << endl;
			return;
		}
	}
	else {
		cout << "Failed to parse file type!" << endl;
	}
	send(s, "\r\n", 2, 0);
}

//发送文件给客户端
void send_file(SOCKET s, char* filename) {
	int flag;
	FILE* pfile;
	errno_t error = fopen_s(&pfile, filename, "rb");

	if (error) {
		cout << "Fail to open file!" << endl;
		return;
	}
	fseek(pfile, 0L, SEEK_END);
	int file_len = ftell(pfile);
	char* p = (char*)malloc(file_len + 1);
	fseek(pfile, 0L, SEEK_SET);
	fread(p, file_len, 1, pfile);
	send(s, p, file_len, 0);
}

void main() {
	WSADATA wsaData;
	/*
		select()机制中提供的fd_set的数据结构，实际上是long类型的数组，
		每一个数组元素都能与一打开的文件句柄（不管是socket句柄，还是其他文件或命名管道或设备句柄）建立联系，建立联系的工作由程序员完成.
		当调用select()时，由内核根据IO状态修改fd_set的内容，由此来通知执行了select()的进程哪个socket或文件句柄发生了可读或可写事件。
	*/
	fd_set rfds;				//用于检查socket是否有数据到来的的文件描述符，用于socket非阻塞模式下等待网络事件通知（有数据到来）
	fd_set wfds;				//用于检查socket是否可以发送的文件描述符，用于socket非阻塞模式下等待网络事件通知（可以发送数据）
	bool first_connetion = true;
	/*初始化Win Socket环境*/
	int nRc = WSAStartup(0x0202, &wsaData);

	if (nRc) {
		printf("Winsock  startup failed with error!\n");
	}

	if (wsaData.wVersion != 0x0202) {
		printf("Winsock version is not correct!\n");
	}

	printf("Winsock  startup Ok!\n");

	/*创建监听socket*/
	SOCKET srvSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (srvSocket != INVALID_SOCKET)
		cout << "Socket create OK!" << endl;

	/*监听socket*/
	int port = 0;
	string inaddr;
	char Home[48] = "";
	cout << "Please input port number:" << endl;
	cin >> port;
	cout << "Please input listening address:" << endl;
	cin >> inaddr;
	cout << "Please input Home directory:" << endl;
	cin >> Home;

	/*处理IP地址长度*/
	const char* p = inaddr.c_str();

	/*设置服务器的端口与地址*/
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;  //设置协议为ipv4
	addr.sin_port = htons(port);  //处理端口号
	addr.sin_addr.s_addr = inet_addr(p);  //处理监听地址

	/*binding*/
	int rtn = bind(srvSocket, (LPSOCKADDR)&addr, sizeof(addr));
	if (rtn != SOCKET_ERROR)
		cout << "Socket bind OK!" << endl;
	else {
		cout << "Socket binding Error!" << endl;
		return ;
	}
	/*监听*/
	rtn = listen(srvSocket, 5);
	if (rtn != SOCKET_ERROR)
		cout << "Socket listen OK!" << endl;
	else cout << "Socket listen Error!" << endl;

	/*创建客户端地址*/
	struct sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);

	while (true) {
		/*等待连接，创建会话socket*/
		SOCKET sessionSocket = accept(srvSocket, (LPSOCKADDR)&clientAddr, &clientAddrLen);
		if (sessionSocket != INVALID_SOCKET)
			cout << "Socket listen one client request!" << endl;

		cout << "The IP and port number of the client are：" << inet_ntoa(clientAddr.sin_addr) << htons(clientAddr.sin_port) << endl<<endl;

		/*开始会话*/
		char recvBuf[4096] = ""; //设置缓冲区
		rtn = recv(sessionSocket, recvBuf, 4096, 0);
		if (rtn != SOCKET_ERROR)
			cout << recvBuf << "Received " << strlen(recvBuf) << " bytes of data from client." << endl;

		/*处理输入的文件名*/
		char name[30] = "";

		int i = find(recvBuf, recvBuf + strlen(recvBuf), '/') - recvBuf;
		int j = 0;
		while (recvBuf[i + 1] != ' ') {
			name[j] = recvBuf[i + 1];
			i++;
			j++;
		}
		name[j] = '\0';
		cout << "File name is：" << name << endl;

		/*处理文件路径*/
		char path[80] = "";
		if (Home[strlen(Home) - 1] != '\\') {
			Home[strlen(Home)] = '\\';
		}
		strcpy_s(path, sizeof(path) / sizeof(path[0]), Home);
		strcat_s(path, strlen(path) + strlen(name) + 1, name);
		cout << "File path is：" << path << endl;

		/*发送响应报文头部及文件*/
		send_head(path, sessionSocket, path);
		send_file(sessionSocket, path);

		/*关闭sessionSocket*/
		closesocket(sessionSocket);
	}

	/*关闭srvSocket*/
	closesocket(srvSocket);
	WSACleanup();

	getchar();
	return;
}

