#include "stdafx.h"
#include "Global.h"
#include "GBNRdtSender.h"
#include <deque>
#include <iostream>


GBNRdtSender::GBNRdtSender() : base(0), expectSequenceNumberSend(0), waitingState(false)
{

}


GBNRdtSender::~GBNRdtSender()
{
}



bool GBNRdtSender::getWaitingState() {
	return waitingState;//���ص�ǰ�ȴ�״̬
}




bool GBNRdtSender::send(const Message& message) {
	if (this->getWaitingState()) { //���ͷ����ڵȴ�ȷ��״̬
		return false;
	}

	if (initflag) {
		for (int i = 0; i < Winlength; i++) {
			this->packetWaitingAck[i].seqnum = -1; //�ȴ��е����ݰ��������Ϊ-1
		}
		initflag = false;
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
			pns->startTimer(SENDER, Configuration::TIME_OUT, base);  //�������ͻ����з���ʱ��
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

void GBNRdtSender::receive(const Packet& ackPkt) {

	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//���У�����ȷ������ȷ����Ų��Ƿ��ͷ���ȷ�ϵ����ݰ����
	if (checkSum == ackPkt.checksum && ackPkt.acknum >= base) {

		int based = base;
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
		base = ackPkt.acknum + 1;

		for (int i = base+N; i < base+Winlength; i++) {
			packetWaitingAck[i % Winlength].seqnum = -1;
		}
		cout << "���ͷ������������ݣ�" << '['<<' ';

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
			cout << "�ѷ��͵ķ����Ѿ�ȫ�����ͣ��رռ�ʱ����" << endl;
			this->waitingState = false;
			pns->stopTimer(SENDER, based);
		}
		else {
			pns->stopTimer(SENDER, based);
			pns->startTimer(SENDER, Configuration::TIME_OUT, base);//��δ�����꣬�����ȴ�
			this->waitingState = false;
		}

	}
	else {
		if (checkSum != ackPkt.checksum) {
			cout << "���ͷ��յ���ACK�𻵣�" << endl;
		}
		else {
			cout << "���ͷ�û���յ���ȷ����ţ������ȴ���" << endl;
		}
	}
}

void GBNRdtSender::timeoutHandler(int seqNum) {
	//Ψһһ����ʱ��,���迼��seqNum
	cout << "���ͳ�ʱ������!" << endl;
	//���·������ݰ�
	pns->stopTimer(SENDER,seqNum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT,seqNum);			//�����������ͷ���ʱ��
	//printf("����seqnum=%d�ļ�ʱ��\n", window->front().seqnum);
	int i = base;
	do {
		cout << "�ط�" << i << "�ű���" << endl;
		pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط����ģ�", this->packetWaitingAck[i % Winlength]);
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[i % Winlength]);
		i++;
	} while (i != expectSequenceNumberSend);

}


