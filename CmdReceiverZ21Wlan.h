/*
 * CmdReceiverZ21Wlan.h
 *
 *  Created on: 18.08.2016
 *      Author: sven
 */

#ifndef CMDRECEIVERZ21WLAN_H_
#define CMDRECEIVERZ21WLAN_H_

#include "CmdSenderBase.h"
#include "z21PaketParser.h"
#include <WiFiClient.h>
#include <WiFiUdp.h>

class CmdReceiverZ21Wlan: public CmdSenderBase, public z21PaketParser {
public:
	CmdReceiverZ21Wlan(Controller* c, const char* ip);
	virtual int loop();
	virtual ~CmdReceiverZ21Wlan();
	void requestTurnoutInfo(int addr);
	void requestLocoInfo(int addr);
	void enableBroadcasts();
	virtual void sendSetTurnout(String id, String status);

private:
	WiFiUDP* udp;
	unsigned int localPort = 21105;
	const int packetBufferSize = 30;
	unsigned char packetBuffer[30];
	unsigned char packetcfg12[30];
	unsigned char packetcfg16[30];
	unsigned long timeout = 0;
	IPAddress* z21Server;
	void doReceive();
	void resetTimeout();

	void sendLanGetSerialNumber();
	void sendCfg12Request();
	void sendCfg16Request();
	void sendFrimwareVersionRequest();
	void sendXGetStatus();
	void sendGetBroadcastFlags();

	void requestRailcom();

	long int lastTime = 0;
	static const int emergencyStopTimeout = 1000;
	const int firstLoopStatus = -4;
	int loopStatus = firstLoopStatus;
};

#endif /* CMDRECEIVERZ21WLAN_H_ */
