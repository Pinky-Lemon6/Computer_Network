#ifndef STOP_WAIT_RDT_SENDER_H
#define STOP_WAIT_RDT_SENDER_H
#include "RdtSender.h"
class TCPRdtSender :public RdtSender
{
private:
	int base;
	int expectSequenceNumberSend;	// 下一个发送序号
	bool initFlag = true;
	int ACKCount = 0;  //收到的ACK的数量
	int lastACK = -1; //上一个ACK数据包序号
	bool waitingState;				// 是否处于等待Ack的状态
	Packet packetWaitingAck[N];		//已发送并等待Ack的数据包
public:

	bool getWaitingState();
	bool send(const Message& message);						//发送应用层下来的Message，由NetworkServiceSimulator调用,如果发送方成功地将Message发送到网络层，返回true;如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
	void receive(const Packet& ackPkt);						//接受确认Ack，将被NetworkServiceSimulator调用	
	void timeoutHandler(int seqNum);					//Timeout handler，将被NetworkServiceSimulator调用

public:
	TCPRdtSender();
	virtual ~TCPRdtSender();
};

#endif