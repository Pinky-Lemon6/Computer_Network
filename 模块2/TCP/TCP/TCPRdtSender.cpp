#include "stdafx.h"
#include "Global.h"
#include "TCPRdtSender.h"
#include <deque>
#include <iostream>


TCPRdtSender::TCPRdtSender() : base(0), expectSequenceNumberSend(0), waitingState(false)
{
}


TCPRdtSender::~TCPRdtSender()
{
}



bool TCPRdtSender::getWaitingState() {
	return waitingState;//���ص�ǰ�ȴ�״̬
}




bool TCPRdtSender::send(const Message& message) {
	if (this->waitingState) { //���ͷ����ڵȴ�ȷ��״̬
		return false;
	}
	if (initFlag) {
		for (int i = 0; i < N; i++) {
			this->packetWaitingAck[i].seqnum = -1;
		}
		initFlag = false;
	}

	if (expectSequenceNumberSend < base + Winlength) {
		this->packetWaitingAck[expectSequenceNumberSend % N].acknum = -1; //���Ը��ֶ�
		this->packetWaitingAck[expectSequenceNumberSend % N].seqnum = this->expectSequenceNumberSend;
		this->packetWaitingAck[expectSequenceNumberSend % N].checksum = 0;
		memcpy(this->packetWaitingAck[expectSequenceNumberSend % N].payload, message.data, sizeof(message.data));
		this->packetWaitingAck[expectSequenceNumberSend % N].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[expectSequenceNumberSend % N]); //����У���

		pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck[expectSequenceNumberSend % N]);

		if (base == expectSequenceNumberSend) {
			cout << "���ͷ�������ʱ��" << endl;
			pns->startTimer(SENDER, Configuration::TIME_OUT, expectSequenceNumberSend);  //�������ͻ����з���ʱ��
		}
		
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend % N]);  //����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
		expectSequenceNumberSend++;

		cout << "������Ϻ�expectSequenceNumberSendΪ��" << expectSequenceNumberSend << endl;
		if (expectSequenceNumberSend == base + Winlength) {
			this->waitingState = true;  //����ȴ�״̬
		}
	}
	return true;
}

void TCPRdtSender::receive(const Packet& ackPkt) {

	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//���У�����ȷ������ȷ������Ƿ��ͷ��ѷ��Ͳ��ȴ�ȷ�ϵ����ݰ����
	if (checkSum == ackPkt.checksum && ackPkt.acknum >= base) {
		int based = base;
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ�ϣ�", ackPkt);

		base = ackPkt.acknum+1;
		for (int i = base + Winlength; i < base + N; i++) {
			packetWaitingAck[i % N].seqnum = -1;  //��ǲ��ڴ����еı������
		}
		cout << "���ͷ��������ڵ�����Ϊ��" << '[' << ' ';
		for (int i = base; i < base+Winlength; i++) {
			if (packetWaitingAck[i % N].seqnum == -1) {
				cout << '*' << ' ';
			}
			else {
				cout << packetWaitingAck[i % N].seqnum << ' ';
			}
		}
		cout << ']' << endl;
		if (base == expectSequenceNumberSend) { //ȫ������
			cout << "�ѷ��͵ķ�����ȫ�����գ��رռ�ʱ����" << endl;
			this->waitingState = false;
			pns->stopTimer(SENDER, based); //�رռ�ʱ��
		}
		else {
			pns->stopTimer(SENDER, based); //��δ�����꣬�����ȴ�
			pns->startTimer(SENDER, Configuration::TIME_OUT, base);
			this->waitingState = false;
		}
	}
	else {
		if (ackPkt.acknum == lastACK) { 
			ACKCount++;
			if (ACKCount == 4) { //�����ش�
				cout << "�յ������������ACK�������ش����Ϊ��" << ackPkt.acknum + 1 <<"�ı���" << endl;
				pns->stopTimer(SENDER, ackPkt.acknum + 1);
				pns->startTimer(SENDER, Configuration::TIME_OUT, ackPkt.acknum + 1);
				pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[base % N]);   //����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�

			}
		}
		else {
			lastACK = ackPkt.acknum; //������һ��ȷ�ϵ����
			ACKCount = 1;
		}
		
		if (checkSum != ackPkt.checksum) {
			cout << "���ͷ��յ���ACK��" << endl;
		}
		else {
			cout << "���ͷ�û���յ���ȷ����ţ������ȴ�" << endl;
		}
	}

}

void TCPRdtSender::timeoutHandler(int seqNum) {
	//Ψһһ����ʱ��,���迼��seqNum
	cout << "���ͳ�ʱ!" << endl;
	//���·������ݰ�
	pns->stopTimer(SENDER,seqNum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT,seqNum);			//�����������ͷ���ʱ��
	//printf("����seqnum=%d�ļ�ʱ��\n", window->front().seqnum);
	cout << "�ط�" << seqNum << "�ű���" << endl;
	pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط����ģ�", this->packetWaitingAck[seqNum % N]);
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[seqNum % N]);
}


