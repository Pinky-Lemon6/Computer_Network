#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"
#include <deque>
#include <iostream>


SRRdtSender::SRRdtSender() : base(0), expectSequenceNumberSend(0), waitingState(false)
{
	for (int i = 0; i < Winlength; i++) {
		ACKStatus[i] = false;           //初始化等待状态
	}
}


SRRdtSender::~SRRdtSender()
{
}



bool SRRdtSender::getWaitingState() {
	return waitingState;//返回当前等待状态
}




bool SRRdtSender::send(const Message& message) {
	if (this->getWaitingState()) { //发送方处于等待确认状态
		return false;
	}


	if (expectSequenceNumberSend < base + N) {

		this->packetWaitingAck[expectSequenceNumberSend % Winlength].acknum = -1; //忽略该字段
		this->packetWaitingAck[expectSequenceNumberSend % Winlength].seqnum = this->expectSequenceNumberSend;
		this->packetWaitingAck[expectSequenceNumberSend % Winlength].checksum = 0;
		memcpy(this->packetWaitingAck[expectSequenceNumberSend % Winlength].payload, message.data, sizeof(message.data));
		this->packetWaitingAck[expectSequenceNumberSend % Winlength].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[expectSequenceNumberSend % Winlength]);
		ACKStatus[expectSequenceNumberSend % Winlength] = false;

		pUtils->printPacket("发送方发送报文", this->packetWaitingAck[expectSequenceNumberSend % Winlength]);

		cout <<"序列"<< expectSequenceNumberSend<<"发送方启动计时器" << endl;
		pns->startTimer(SENDER, Configuration::TIME_OUT, expectSequenceNumberSend);  //启动发送方定时器
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend % Winlength]);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
		expectSequenceNumberSend++;

		cout << "发送完毕后，expectSequenceNumberSend为：" << expectSequenceNumberSend << endl;
		if (expectSequenceNumberSend == base + N) {
			this->waitingState = true;  //进入等待状态
		}
	}
	return true;
}

void SRRdtSender::receive(const Packet& ackPkt) {

	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//如果校验和正确，并且确认序号是发送方已发送并等待确认的数据包序号
	if (checkSum == ackPkt.checksum ) {
		pUtils->printPacket("发送方正确收到确认！", ackPkt);

		for (int i = base+N; i < base+Winlength; i++) {
			packetWaitingAck[i % Winlength].seqnum = -1;
		}
		cout << "发送方滑动窗口内容：" << '['<<' ';

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
			cout << "已确认ACK序号为：" <<ackPkt.acknum<<"的ACK" << endl;
			pns->stopTimer(SENDER, ackPkt.acknum);  //停止计时
			ACKStatus[base % Winlength] = true;
			while (ACKStatus[base % Winlength]) {
				ACKStatus[base++ % Winlength] = false;  //若已确认ACK序号则将其ACK状态设为false
			}
			waitingState = false; //结束等待
		}
		else if(ackPkt.acknum>base && !ACKStatus[ackPkt.acknum % Winlength]) {
			cout << "已确认ACK序号为：" << ackPkt.acknum << "的ACK" << endl;
			pns->stopTimer(SENDER, ackPkt.acknum);
			ACKStatus[ackPkt.acknum % Winlength] = true;  //接收尚未结束，继续等待
		}
		else {
			cout << "接收到了不需要的序列的ACK，继续等待！" << endl;
		}
	}
	else {
			cout << "发送方收到的ACK损坏！" << endl;
		}

}

void SRRdtSender::timeoutHandler(int seqNum) {
	//唯一一个定时器,无需考虑seqNum
	cout << "发送超时!" << endl;
	//重新发送数据包
	pns->stopTimer(SENDER,seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT,seqNum);			//重新启动发送方定时器
	//printf("启动seqnum=%d的计时器\n", window->front().seqnum);
	cout << "重发" << seqNum << "号报文" << endl;
	pUtils->printPacket("发送方定时器时间到，重发报文！", this->packetWaitingAck[seqNum % Winlength]);
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[seqNum % Winlength]);
}


