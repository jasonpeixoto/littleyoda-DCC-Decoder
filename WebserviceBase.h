/*
 * WebserviceBase.h
 *
 *  Created on: 21.08.2016
 *      Author: sven
 */

#ifndef WEBSERVICEBASE_H_
#define WEBSERVICEBASE_H_
#include "Arduino.h"

class ESP8266WebServer;

class WebserviceBase {
public:
	WebserviceBase();
	virtual ~WebserviceBase();
	virtual char const* getUri() = 0;
	virtual void run() = 0;
	void setServer(ESP8266WebServer* server);
	virtual String  getLinkText();


protected:
	ESP8266WebServer* server;
	void sendBasicHeader();
	void finishSend();
	void send(const String& content);

};

#endif /* WEBSERVICEBASE_H_ */
