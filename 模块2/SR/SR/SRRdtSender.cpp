#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"
#include <deque>
#include <iostream>


SRRdtSender::SRRdtSender() : base(0), expectSequenceNumberSend(0), waitingState(false)
{
	for (int i = 0; i < N; i++) {
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
	if (this->waitingState) { //���ͷ����ڵȴ�ȷ��״̬
		return false;
	}


	if (expectSequenceNumberSend < base + Winlength) {
		this->packetWaitingAck[expectSequenceNumberSend % N].acknum = -1; //���Ը��ֶ�
		this->packetWaitingAck[expectSequenceNumberSend % N].seqnum = this->expectSequenceNumberSend;
		this->packetWaitingAck[expectSequenceNumberSend % N].checksum = 0;
		memcpy(this->packetWaitingAck[expectSequenceNumberSend % N].payload, message.data, sizeof(message.data));
		this->packetWaitingAck[expectSequenceNumberSend % N].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[expectSequenceNumberSend % N]); //����У���
		ACKStatus[expectSequenceNumberSend % N] = false;

		pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck[expectSequenceNumberSend % N]);

		cout <<"����"<< expectSequenceNumberSend<<"���ͷ�������ʱ��" << endl;
		pns->startTimer(SENDER, Configuration::TIME_OUT, expectSequenceNumberSend);  //�������ͷ���ʱ��
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend % N]);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
		expectSequenceNumberSend++;

		cout << "������Ϻ�expectSequenceNumberSendΪ��" << expectSequenceNumberSend << endl;
		if (expectSequenceNumberSend == base + Winlength) {
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

		for (int i = base+Winlength; i < base+N; i++) {
			packetWaitingAck[i % N].seqnum = -1;  //�Բ��ڴ����еı�����Ž��б��
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
		if (base == ackPkt.acknum) {
			cout << "��ȷ��ACK���Ϊ��" <<ackPkt.acknum<<"��ACK" << endl;
			pns->stopTimer(SENDER, ackPkt.acknum);  //ֹͣ��ʱ
			ACKStatus[base % N] = true;  //��ȷ��ACK���
			while (ACKStatus[base % N]) {
				ACKStatus[base++ % N] = false;  //����ȷ��ACK�������ACK״̬��Ϊfalse
			}
			waitingState = false; //�����ȴ�
		}
		else if(ackPkt.acknum>base && !ACKStatus[ackPkt.acknum % N]) { 
			cout << "��ȷ��ACK���Ϊ��" << ackPkt.acknum << "��ACK" << endl;
			pns->stopTimer(SENDER, ackPkt.acknum);
			ACKStatus[ackPkt.acknum % N] = true;  //������δ�����������ȴ�
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
	pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط����ģ�", this->packetWaitingAck[seqNum % N]);
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[seqNum % N]);
}


