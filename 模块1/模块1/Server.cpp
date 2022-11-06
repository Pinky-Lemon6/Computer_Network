#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#include "winsock2.h"
#include <stdio.h>
#include <iostream>
using namespace std;

#pragma comment(lib,"ws2_32.lib")


//��λ�ļ�����׺
char* file_type_addr(char* argue) {
	char* temp;
	if ((temp = strchr(argue, '.')) != NULL) {
		return temp + 1;
	}
	return (char*)"";
}

//���첢������Ӧ����ͷ��
void send_head(char* argue, SOCKET s, char* filename) {
	/*�����ļ�*/
	char* filetype = file_type_addr(argue);
	char* content_type = (char*)"text/plain";
	char* body_length = (char*)"Content-Length��";

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


	/*������Ӧ����ͷ��*/
	char* head = (char*)"HTTP/1.1 200 OK\r\n";
	char* not_found = (char*)"HTTP/1.1 404 NOT FOUND\r\n";
	char temp[50] = "Content-type��";


	/*��ȡ�ļ�������*/
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

//�����ļ����ͻ���
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
	string inaddr;
	char Home[48] = "";
	cout << "Please input port number:" << endl;
	cin >> port;
	cout << "Please input listening address:" << endl;
	cin >> inaddr;
	cout << "Please input Home directory:" << endl;
	cin >> Home;

	/*����IP��ַ����*/
	const char* p = inaddr.c_str();

	/*���÷������Ķ˿����ַ*/
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;  //����Э��Ϊipv4
	addr.sin_port = htons(port);  //����˿ں�
	addr.sin_addr.s_addr = inet_addr(p);  //���������ַ

	/*binding*/
	int rtn = bind(srvSocket, (LPSOCKADDR)&addr, sizeof(addr));
	if (rtn != SOCKET_ERROR)
		cout << "Socket bind OK!" << endl;
	else {
		cout << "Socket binding Error!" << endl;
		return ;
	}
	/*����*/
	rtn = listen(srvSocket, 5);
	if (rtn != SOCKET_ERROR)
		cout << "Socket listen OK!" << endl;
	else cout << "Socket listen Error!" << endl;

	/*�����ͻ��˵�ַ*/
	struct sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);

	while (true) {
		/*�ȴ����ӣ������Ựsocket*/
		SOCKET sessionSocket = accept(srvSocket, (LPSOCKADDR)&clientAddr, &clientAddrLen);
		if (sessionSocket != INVALID_SOCKET)
			cout << "Socket listen one client request!" << endl;

		cout << "The IP and port number of the client are��" << inet_ntoa(clientAddr.sin_addr) << htons(clientAddr.sin_port) << endl<<endl;

		/*��ʼ�Ự*/
		char recvBuf[4096] = ""; //���û�����
		rtn = recv(sessionSocket, recvBuf, 4096, 0);
		if (rtn != SOCKET_ERROR)
			cout << recvBuf << "Received " << strlen(recvBuf) << " bytes of data from client." << endl;

		/*����������ļ���*/
		char name[30] = "";

		int i = find(recvBuf, recvBuf + strlen(recvBuf), '/') - recvBuf;
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
		if (Home[strlen(Home) - 1] != '\\') {
			Home[strlen(Home)] = '\\';
		}
		strcpy_s(path, sizeof(path) / sizeof(path[0]), Home);
		strcat_s(path, strlen(path) + strlen(name) + 1, name);
		cout << "File path is��" << path << endl;

		/*������Ӧ����ͷ�����ļ�*/
		send_head(path, sessionSocket, path);
		send_file(sessionSocket, path);

		/*�ر�sessionSocket*/
		closesocket(sessionSocket);
	}

	/*�ر�srvSocket*/
	closesocket(srvSocket);
	WSACleanup();

	getchar();
	return;
}

