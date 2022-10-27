#pragma once
#include "winsock2.h"
#include <stdio.h>
#include <iostream>
using namespace std;

#pragma comment(lib,"ws2_32.lib")

void main() {
	WSADATA wsaData;
	/*
		select()�������ṩ��fd_set�����ݽṹ��ʵ������long���͵����飬
		ÿһ������Ԫ�ض�����һ�򿪵��ļ������������socket��������������ļ��������ܵ����豸�����������ϵ��������ϵ�Ĺ����ɳ���Ա���.
		������select()ʱ�����ں˸���IO״̬�޸�fd_set�����ݣ��ɴ���ִ֪ͨ����select()�Ľ����ĸ�socket���ļ���������˿ɶ����д�¼���
	*/
	fd_set rfds;				//���ڼ��socket�Ƿ������ݵ����ĵ��ļ�������������socket������ģʽ�µȴ������¼�֪ͨ�������ݵ�����
	fd_set wfds;				//���ڼ��socket�Ƿ���Է��͵��ļ�������������socket������ģʽ�µȴ������¼�֪ͨ�����Է������ݣ�
	bool first_connetion = true;
	/*��ʼ��Win Socket����*/
	int nRc = WSAStartup(0x0202, &wsaData);

	if (nRc) {
		printf("Winsock  startup failed with error!\n");
	}

	if (wsaData.wVersion != 0x0202) {
		printf("Winsock version is not correct!\n");
	}

	printf("Winsock  startup Ok!\n");

	/*��������socket*/
	SOCKET srvSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (srvSocket != INVALID_SOCKET)
		cout << "Socket create OK!" << endl;

	/*����socket*/
	int port = 0;
	char inaddr[20] = "";
	char filename[48] = "";
	cout << "Please input port number:" << endl;
	cin >> port;
	cout << "Please input listening address:" << endl;
	cin >> inaddr;
	cout << "Please input Home directory:" << endl;
	cin >> filename;

	/*���÷������Ķ˿����ַ*/
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;  //����Э��Ϊipv4
	addr.sin_port = htons(port);  //����˿ں�
	addr.sin_addr.s_addr = inet_addr(inaddr);  //���������ַ

	/*binding*/
	int rtn = bind(srvSocket, (LPSOCKADDR) & addr, sizeof(addr));
	if (rtn != SOCKET_ERROR)
		cout << "Socket bind OK!" << endl;

	/*����*/
	rtn = listen(srvSocket, 5);
	if (rtn != SOCKET_ERROR)
		cout << "Socket listen OK!" << endl;

	/*�����ͻ��˵�ַ*/
	struct sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);

	while (true) {
		/*�ȴ����ӣ������Ựsocket*/
		SOCKET sessionSocket = accept(srvSocket,(LPSOCKADDR) & clientAddr, &clientAddrLen);
		if (sessionSocket != INVALID_SOCKET)
			cout << "Socket listen one client request!" << endl;
		
		cout << "The IP and port number of the client are��" << inet_ntoa(clientAddr.sin_addr) << htons(clientAddr.sin_port) << endl;
		
		/*��ʼ�Ự*/
		char recvBuf[4096]; //���û�����
		rtn = recv(sessionSocket, recvBuf, 1024, 0);
		if (rtn != SOCKET_ERROR)
			cout << "Received "<<strlen(recvBuf)<<" bytes from client:"<<recvBuf<< endl;

		/*����������ļ���*/
		char name[30] = "";
		
		int i = find(recvBuf, recvBuf+strlen(recvBuf), '/')-recvBuf;
		int j = 0;
		while (recvBuf[i + 1] != ' ') {
			name[j] = recvBuf[i + 1];
			i++;
			j++;
		}
		name[j] = '\0';
		cout << "File name is��" << name << endl;

		/*�����ļ�·��*/
		char path[80] = "";
		strcpy(path, filename);
		strcat(path, name);
		cout << "File path is��" << path << endl;



	}






}