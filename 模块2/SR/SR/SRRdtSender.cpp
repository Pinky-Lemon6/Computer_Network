#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"
#include <deque>
#include <iostream>


SRRdtSender::SRRdtSender() : base(0), expectSequenceNumberSend(0), waitingState(false)
{
	for (int i = 0; i < Winlength; i++) {
		ACKStatus[i] = false;           //��ʼ���ȴ�״̬
	}
}


SRRdtSender::~SRRdtSender()
{
}



bool SRRdtSender::getWaitingState() {
	return waitingState;//���ص�ǰ�ȴ�״̬
}




bool SRRdtSender::send(const Message& message) {
	if (this->getWaitingState()) { //���ͷ����ڵȴ�ȷ��״̬
		return false;
	}


	if (expectSequenceNumberSend < base + N) {

		this->packetWaitingAck[expectSequenceNumberSend % Winlength].acknum = -1; //���Ը��ֶ�
		this->packetWaitingAck[expectSequenceNumberSend % Winlength].seqnum = this->expectSequenceNumberSend;
		this->packetWaitingAck[expectSequenceNumberSend % Winlength].checksum = 0;
		memcpy(this->packetWaitingAck[expectSequenceNumberSend % Winlength].payload, message.data, sizeof(message.data));
		this->packetWaitingAck[expectSequenceNumberSend % Winlength].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[expectSequenceNumberSend % Winlength]);
		ACKStatus[expectSequenceNumberSend % Winlength] = false;

		pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck[expectSequenceNumberSend % Winlength]);

		cout <<"����"<< expectSequenceNumberSend<<"���ͷ�������ʱ��" << endl;
		pns->startTimer(SENDER, Configuration::TIME_OUT, expectSequenceNumberSend);  //�������ͷ���ʱ��
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend % Winlength]);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
		expectSequenceNumberSend++;

		cout << "������Ϻ�expectSequenceNumberSendΪ��" << expectSequenceNumberSend << endl;
		if (expectSequenceNumberSend == base + N) {
			this->waitingState = true;  //����ȴ�״̬
		}
	}
	return true;
}

void SRRdtSender::receive(const Packet& ackPkt) {

	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//���У�����ȷ������ȷ������Ƿ��ͷ��ѷ��Ͳ��ȴ�ȷ�ϵ����ݰ����
	if (checkSum == ackPkt.checksum ) {
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ�ϣ�", ackPkt);

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
		if (base == ackPkt.acknum) {
			cout << "��ȷ��ACK���Ϊ��" <<ackPkt.acknum<<"��ACK" << endl;
			pns->stopTimer(SENDER, ackPkt.acknum);  //ֹͣ��ʱ
			ACKStatus[base % Winlength] = true;
			while (ACKStatus[base % Winlength]) {
				ACKStatus[base++ % Winlength] = false;  //����ȷ��ACK�������ACK״̬��Ϊfalse
			}
			waitingState = false; //�����ȴ�
		}
		else if(ackPkt.acknum>base && !ACKStatus[ackPkt.acknum % Winlength]) {
			cout << "��ȷ��ACK���Ϊ��" << ackPkt.acknum << "��ACK" << endl;
			pns->stopTimer(SENDER, ackPkt.acknum);
			ACKStatus[ackPkt.acknum % Winlength] = true;  //������δ�����������ȴ�
		}
		else {
			cout << "���յ��˲���Ҫ�����е�ACK�������ȴ���" << endl;
		}
	}
	else {
			cout << "���ͷ��յ���ACK�𻵣�" << endl;
		}

}

void SRRdtSender::timeoutHandler(int seqNum) {
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


