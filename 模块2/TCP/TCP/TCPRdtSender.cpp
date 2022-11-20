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
		for (int i = 0; i < Winlength; i++) {
			this->packetWaitingAck[i].seqnum = -1;
		}
		initFlag = false;
	}

	if (expectSequenceNumberSend < base + N) {
		this->packetWaitingAck[expectSequenceNumberSend % Winlength].acknum = -1; //���Ը��ֶ�
		this->packetWaitingAck[expectSequenceNumberSend % Winlength].seqnum = this->expectSequenceNumberSend;
		this->packetWaitingAck[expectSequenceNumberSend % Winlength].checksum = 0;
		memcpy(this->packetWaitingAck[expectSequenceNumberSend % Winlength].payload, message.data, sizeof(message.data));
		this->packetWaitingAck[expectSequenceNumberSend % Winlength].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[expectSequenceNumberSend % Winlength]);

		pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck[expectSequenceNumberSend % Winlength]);

		if (base == expectSequenceNumberSend) {
			cout << "���ͷ�������ʱ��" << endl;
			pns->startTimer(SENDER, Configuration::TIME_OUT, expectSequenceNumberSend);  //�������ͻ����з���ʱ��
		}
		
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend % Winlength]);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
		expectSequenceNumberSend++;

		cout << "������Ϻ�expectSequenceNumberSendΪ��" << expectSequenceNumberSend << endl;
		if (expectSequenceNumberSend == base + N) {
			this->waitingState = true;  //����ȴ�״̬
		}
	}
	return true;
}

void TCPRdtSender::receive(const Packet& ackPkt) {

	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//���У�����ȷ������ȷ������Ƿ��ͷ��ѷ��Ͳ��ȴ�ȷ�ϵ����ݰ����
	if (checkSum == ackPkt.checksum ) {
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ�ϣ�", ackPkt);
		int based = base;
		base = ackPkt.acknum+1;
		for (int i = base + N; i < base + Winlength; i++) {
			packetWaitingAck[i % Winlength].seqnum = -1;  //��ǲ��ڴ����е����
		}
		cout << "���ͷ��������ڵ�����Ϊ��" << '[' << ' ';
		for (int i = base; i < base+N; i++) {
			if (packetWaitingAck[i % Winlength].seqnum == -1) {
				cout << '*' << ' ';
			}
			else {
				cout << packetWaitingAck[i % Winlength].seqnum << ' ';
			}
		}
		cout << ']' << endl;
		if (base == expectSequenceNumberSend) {
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
			if (ACKCount == 4) {
				cout << "�յ������������ACK�������ش���ţ�" << ackPkt.acknum + 1 << endl;
				pns->stopTimer(SENDER, ackPkt.acknum + 1);
				pns->startTimer(SENDER, Configuration::TIME_OUT, ackPkt.acknum + 1);
				pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[base % Winlength]);

			}
		}
		else {
			lastACK = ackPkt.acknum;
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
	pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط����ģ�", this->packetWaitingAck[seqNum % Winlength]);
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[seqNum % Winlength]);
}


