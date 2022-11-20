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
	return waitingState;//返回当前等待状态
}




bool TCPRdtSender::send(const Message& message) {
	if (this->waitingState) { //发送方处于等待确认状态
		return false;
	}
	if (initFlag) {
		for (int i = 0; i < Winlength; i++) {
			this->packetWaitingAck[i].seqnum = -1;
		}
		initFlag = false;
	}

	if (expectSequenceNumberSend < base + N) {
		this->packetWaitingAck[expectSequenceNumberSend % Winlength].acknum = -1; //忽略该字段
		this->packetWaitingAck[expectSequenceNumberSend % Winlength].seqnum = this->expectSequenceNumberSend;
		this->packetWaitingAck[expectSequenceNumberSend % Winlength].checksum = 0;
		memcpy(this->packetWaitingAck[expectSequenceNumberSend % Winlength].payload, message.data, sizeof(message.data));
		this->packetWaitingAck[expectSequenceNumberSend % Winlength].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[expectSequenceNumberSend % Winlength]);

		pUtils->printPacket("发送方发送报文", this->packetWaitingAck[expectSequenceNumberSend % Winlength]);

		if (base == expectSequenceNumberSend) {
			cout << "发送方启动计时器" << endl;
			pns->startTimer(SENDER, Configuration::TIME_OUT, expectSequenceNumberSend);  //启动发送基序列方定时器
		}
		
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend % Winlength]);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
		expectSequenceNumberSend++;

		cout << "发送完毕后，expectSequenceNumberSend为：" << expectSequenceNumberSend << endl;
		if (expectSequenceNumberSend == base + N) {
			this->waitingState = true;  //进入等待状态
		}
	}
	return true;
}

void TCPRdtSender::receive(const Packet& ackPkt) {

	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//如果校验和正确，并且确认序号是发送方已发送并等待确认的数据包序号
	if (checkSum == ackPkt.checksum ) {
		pUtils->printPacket("发送方正确收到确认！", ackPkt);
		int based = base;
		base = ackPkt.acknum+1;
		for (int i = base + N; i < base + Winlength; i++) {
			packetWaitingAck[i % Winlength].seqnum = -1;  //标记不在窗口中的序号
		}
		cout << "发送方滑动窗口的内容为：" << '[' << ' ';
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
			cout << "已发送的分组已全部接收，关闭计时器！" << endl;
			this->waitingState = false;
			pns->stopTimer(SENDER, based); //关闭计时器
		}
		else {
			pns->stopTimer(SENDER, based); //尚未接收完，继续等待
			pns->startTimer(SENDER, Configuration::TIME_OUT, base);
			this->waitingState = false;
		}
	}
	else {
		if (ackPkt.acknum == lastACK) {
			ACKCount++;
			if (ACKCount == 4) {
				cout << "收到了三个冗余的ACK，快速重传序号：" << ackPkt.acknum + 1 << endl;
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
			cout << "发送方收到的ACK损坏" << endl;
		}
		else {
			cout << "发送方没有收到正确的序号，继续等待" << endl;
		}
	}

}

void TCPRdtSender::timeoutHandler(int seqNum) {
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


