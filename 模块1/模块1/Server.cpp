#pragma once
#include "winsock2.h"
#include <stdio.h>
#include <iostream>
using namespace std;

#pragma comment(lib,"ws2_32.lib")

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
	char inaddr[20] = "";
	char filename[48] = "";
	cout << "Please input port number:" << endl;
	cin >> port;
	cout << "Please input listening address:" << endl;
	cin >> inaddr;
	cout << "Please input Home directory:" << endl;
	cin >> filename;

	/*设置服务器的端口与地址*/
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;  //设置协议为ipv4
	addr.sin_port = htons(port);  //处理端口号
	addr.sin_addr.s_addr = inet_addr(inaddr);  //处理监听地址

	/*binding*/
	int rtn = bind(srvSocket, (LPSOCKADDR) & addr, sizeof(addr));
	if (rtn != SOCKET_ERROR)
		cout << "Socket bind OK!" << endl;

	/*监听*/
	rtn = listen(srvSocket, 5);
	if (rtn != SOCKET_ERROR)
		cout << "Socket listen OK!" << endl;

	/*创建客户端地址*/
	struct sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);

	while (true) {
		/*等待连接，创建会话socket*/
		SOCKET sessionSocket = accept(srvSocket,(LPSOCKADDR) & clientAddr, &clientAddrLen);
		if (sessionSocket != INVALID_SOCKET)
			cout << "Socket listen one client request!" << endl;
		
		cout << "The IP and port number of the client are：" << inet_ntoa(clientAddr.sin_addr) << htons(clientAddr.sin_port) << endl;
		
		/*开始会话*/
		char recvBuf[4096]; //设置缓冲区
		rtn = recv(sessionSocket, recvBuf, 1024, 0);
		if (rtn != SOCKET_ERROR)
			cout << "Received "<<strlen(recvBuf)<<" bytes from client:"<<recvBuf<< endl;

		/*处理输入的文件名*/
		char name[30] = "";
		
		int i = find(recvBuf, recvBuf+strlen(recvBuf), '/')-recvBuf;
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
		strcpy(path, filename);
		strcat(path, name);
		cout << "File path is：" << path << endl;



	}






}