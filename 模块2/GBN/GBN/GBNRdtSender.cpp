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
	return waitingState;//返回当前等待状态
}




bool GBNRdtSender::send(const Message& message) {
	if (this->getWaitingState()) { //发送方处于等待确认状态
		return false;
	}

	if (initflag) {
		for (int i = 0; i < N; i++) {
			this->packetWaitingAck[i].seqnum = -1; //等待中的数据包的序号置为-1
		}
		initflag = false;
	}

	if (expectSequenceNumberSend < base + Winlength) {

		this->packetWaitingAck[expectSequenceNumberSend % N].acknum = -1; //忽略该字段
		this->packetWaitingAck[expectSequenceNumberSend % N].seqnum = this->expectSequenceNumberSend;
		this->packetWaitingAck[expectSequenceNumberSend % N].checksum = 0;
		memcpy(this->packetWaitingAck[expectSequenceNumberSend % N].payload, message.data, sizeof(message.data));
		this->packetWaitingAck[expectSequenceNumberSend % N].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[expectSequenceNumberSend % N]);
		

		pUtils->printPacket("发送方发送报文", this->packetWaitingAck[expectSequenceNumberSend % N]);

		if (base == expectSequenceNumberSend) {
			cout << "发送方启动计时器" << endl;
			pns->startTimer(SENDER, Configuration::TIME_OUT, base);  //启动发送基序列方定时器
		}
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend % N]);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
		expectSequenceNumberSend++;
		cout << "发送完毕后，expectSequenceNumberSend为：" << expectSequenceNumberSend << endl;

		if (expectSequenceNumberSend == base + Winlength) {
			this->waitingState = true;  //进入等待状态
		}
	}
	return true;
}

void GBNRdtSender::receive(const Packet& ackPkt) {

	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//如果校验和正确，并且确认序号不是发送方已确认的数据包序号
	if (checkSum == ackPkt.checksum && ackPkt.acknum >= base) {

		int based = base;
		pUtils->printPacket("发送方正确收到确认", ackPkt);
		base = ackPkt.acknum + 1;

		for (int i = base+Winlength; i < base+N; i++) {
			packetWaitingAck[i % N].seqnum = -1;
		}
		cout << "发送方滑动窗口内容：" << '['<<' ';

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
			cout << "已发送的分组已经全部接送，关闭计时器。" << endl;
			this->waitingState = false;
			pns->stopTimer(SENDER, based);
		}
		else {
			pns->stopTimer(SENDER, based);
			pns->startTimer(SENDER, Configuration::TIME_OUT, base);//尚未接收完，继续等待
			this->waitingState = false;
		}

	}
	else {
		if (checkSum != ackPkt.checksum) {
			cout << "发送方收到的ACK损坏！" << endl;
		}
		else {
			cout << "发送方没有收到正确的序号，继续等待！" << endl;
		}
	}
}

void GBNRdtSender::timeoutHandler(int seqNum) {
	//唯一一个定时器,无需考虑seqNum
	cout << "发送超时，回退!" << endl;
	//重新发送数据包
	pns->stopTimer(SENDER,seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT,seqNum);			//重新启动发送方定时器
	//printf("启动seqnum=%d的计时器\n", window->front().seqnum);
	int i = base;
	do {
		cout << "重发" << i << "号报文" << endl;
		pUtils->printPacket("发送方定时器时间到，重发报文！", this->packetWaitingAck[i % N]);
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[i % N]);
		i++;
	} while (i != expectSequenceNumberSend);

}


