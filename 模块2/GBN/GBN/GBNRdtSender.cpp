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
		for (int i = 0; i < N; i++) {
			this->packetWaitingAck[i].seqnum = -1; //�ȴ��е����ݰ��������Ϊ-1
		}
		initflag = false;
	}

	if (expectSequenceNumberSend < base + Winlength) {

		this->packetWaitingAck[expectSequenceNumberSend % N].acknum = -1; //���Ը��ֶ�
		this->packetWaitingAck[expectSequenceNumberSend % N].seqnum = this->expectSequenceNumberSend;
		this->packetWaitingAck[expectSequenceNumberSend % N].checksum = 0;
		memcpy(this->packetWaitingAck[expectSequenceNumberSend % N].payload, message.data, sizeof(message.data));
		this->packetWaitingAck[expectSequenceNumberSend % N].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[expectSequenceNumberSend % N]);
		

		pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck[expectSequenceNumberSend % N]);

		if (base == expectSequenceNumberSend) {
			cout << "���ͷ�������ʱ��" << endl;
			pns->startTimer(SENDER, Configuration::TIME_OUT, base);  //�������ͻ����з���ʱ��
		}
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend % N]);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
		expectSequenceNumberSend++;
		cout << "������Ϻ�expectSequenceNumberSendΪ��" << expectSequenceNumberSend << endl;

		if (expectSequenceNumberSend == base + Winlength) {
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

		for (int i = base+Winlength; i < base+N; i++) {
			packetWaitingAck[i % N].seqnum = -1;
		}
		cout << "���ͷ������������ݣ�" << '['<<' ';

		for (int i = base; i < base+Winlength; i++) {
			if (packetWaitingAck[i % N].seqnum == -1) {
				cout << '*' << ' ';
			}
			else {
				cout << packetWaitingAck[i % N].seqnum << ' ';
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
		pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط����ģ�", this->packetWaitingAck[i % N]);
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[i % N]);
		i++;
	} while (i != expectSequenceNumberSend);

}


