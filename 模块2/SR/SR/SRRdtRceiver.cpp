#include "stdafx.h"
#include "Global.h"
#include "SRRdtReceiver.h"


SRRdtReceiver::SRRdtReceiver() :NextSeqNum(0),base(0)
{
	NextSeqNum = base + Winlength; //��ȡ�ڴ��յ�����һ�����ĵ����
	lastAckPkt.acknum = -1; //��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//���Ը��ֶ�
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = '.';
	}
	for (int i = 0; i < N; i++) {
		packetWaitingStatus[i] = false; //�����ݰ��ĵȴ�״̬��Ϊfalse
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}


SRRdtReceiver::~SRRdtReceiver()
{
}

void SRRdtReceiver::receive(const Packet& packet) {
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(packet);

	//���У�����ȷ��ͬʱ�յ����ĵ���ŵ��ڽ��շ��ڴ��յ��ı������һ��
	if (checkSum == packet.checksum ) {
		cout << "���շ������������ݣ�" << '[' << ' ';
		for (int i = 0; i < Winlength; i++) {
			cout << base + i << ' ';
		}
		cout << ']' << endl;

		if (base == packet.seqnum) { //����ǻ�����
			cout << "���շ������ܵ��ı������Ϊ��" << packet.seqnum << endl;
			pUtils->printPacket("���շ���ȷ�յ����ͷ��ı���", packet);

			lastAckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("���շ�����ȷ�ϱ���", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�

			packetWaitingStatus[packet.seqnum % N] = true;
			ReceivedPacket[packet.seqnum % N] = packet;
			ReceivedPacket[packet.seqnum % N].acknum = 0;
			while (packetWaitingStatus[base % N] ) {
				//ȡ��Message�����ϵݽ���Ӧ�ò�
				Message msg;
				memcpy(msg.data, ReceivedPacket[base%N].payload, sizeof(ReceivedPacket[base % N].payload));
				pns->delivertoAppLayer(RECEIVER, msg);
				packetWaitingStatus[base++ % N] = false; //�ͷŻ�����
				packetWaitingStatus[NextSeqNum++ % N] = false; //���뻺����
				ReceivedPacket[packet.seqnum % N].acknum = -1;
			}
		}
		else if(base<packet.seqnum && packet.seqnum<NextSeqNum) { //��һ���յ����Ҳ��ǻ�����
			cout << "���շ��յ��ı������Ϊ��" << packet.seqnum << endl;
			pUtils->printPacket("���շ��Ѿ����淢�ͷ��ı��ģ�", packet);

			//���뻺����
			ReceivedPacket[packet.seqnum % N] = packet;
			packetWaitingStatus[packet.seqnum % N] = true;

			lastAckPkt.acknum = packet.acknum; // ȷ����ŵ����յ��ı������
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("���շ�����ȷ�ϱ��ģ�", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�
		}
		else if (packet.seqnum>=base-Winlength && packet.seqnum<base) {
			pUtils->printPacket("���շ���ȷ������ȷ�ϵĹ�ʱ���ģ�", packet);
			lastAckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt); //�ϴη��͵ı��ĵ�У���
			pUtils->printPacket("���շ�����ȷ�ϱ��ģ�", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�
		}
		else {
			pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,������Ų���", packet);
			cout << "���շ��ڴ����յ��������" << this->base << "~" << this->NextSeqNum << "�ķ�Χ��" << endl;
		}

	}
	else {
			pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,����У�����", packet);
	}
}