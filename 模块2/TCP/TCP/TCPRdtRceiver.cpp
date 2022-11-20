#include "stdafx.h"
#include "Global.h"
#include "TCPRdtReceiver.h"


TCPRdtReceiver::TCPRdtReceiver() :expectSquenceNumberRcvd(0)
{
	NextSeqNum = base + N; //获取期待收到的下一个报文的序号
	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//忽略该字段
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = '.';
	}
	for (int i = 0; i < Winlength; i++) {
		packetWaitingStatus[i] = false; //将数据包的等待状态设为false
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}


TCPRdtReceiver::~TCPRdtReceiver()
{
}

void TCPRdtReceiver::receive(const Packet& packet) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(packet);

	//如果校验和正确，同时收到报文的序号等于接收方期待收到的报文序号一致
	if (checkSum == packet.checksum ) {
		cout << "接收方滑动窗口内容：" << '[' << ' ';
		for (int i = 0; i < N; i++) {
			cout << base + i << ' ';
		}
		cout << ']' << endl;

		if (base == packet.seqnum) { //如果是基序列
			cout << "接收方所接受到的报文序号为：" << packet.seqnum << endl;
			pUtils->printPacket("接收方正确收到发送方的报文", packet);

			lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("接收方发送确认报文", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方

			packetWaitingStatus[packet.seqnum % Winlength] = true;
			ReceivedPacket[packet.seqnum % Winlength] = packet;
			ReceivedPacket[packet.seqnum % Winlength].acknum = 0;
			while (packetWaitingStatus[base % Winlength] ) {
				//取出Message，向上递交给应用层
				Message msg;
				memcpy(msg.data, ReceivedPacket[base%Winlength].payload, sizeof(ReceivedPacket[base % Winlength].payload));
				pns->delivertoAppLayer(RECEIVER, msg);
				packetWaitingStatus[base++ % Winlength] = false; //释放缓冲区
				packetWaitingStatus[NextSeqNum++ % Winlength] = false; //放入缓冲区
				ReceivedPacket[packet.seqnum % Winlength].acknum = -1;
			}
		}
		else if(base<packet.seqnum && packet.seqnum<NextSeqNum) { //第一次收到，且不是基序列
			cout << "接收方收到的报文序号为：" << packet.seqnum << endl;
			pUtils->printPacket("接收方已经缓存发送方的报文！", packet);

			//存入缓冲区
			ReceivedPacket[packet.seqnum % Winlength] = packet;
			packetWaitingStatus[packet.seqnum % Winlength] = true;

			lastAckPkt.acknum = packet.acknum; // 确认序号等于收到的报文序号
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("接收方发送确认报文！", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
		}
		else if (packet.seqnum>=base-N && packet.seqnum<base) {
			pUtils->printPacket("接收方正确接受已确认的过时报文！", packet);
			lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt); //上次发送的报文的校验和
			pUtils->printPacket("接收方发送确认报文！", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
		}
		else {
			pUtils->printPacket("接收方没有正确收到发送方的报文,报文序号不对", packet);
			cout << "接收方期待接收到的序号在" << this->base << "~" << this->NextSeqNum << "的范围内" << endl;
		}

	}
	else {
			pUtils->printPacket("接收方没有正确收到发送方的报文,数据校验错误", packet);
	}
}