/*
 * ActionLed.h
 *
 *  Created on: 27.08.2016
 *      Author: sven
 */

#ifndef ACTIONLED_H_
#define ACTIONLED_H_

#include <Arduino.h>
#include <LinkedList.h>

#include "ILoop.h"
#include "ISettings.h"
#include "Pin.h"

class ActionLed: public ILoop, public ISettings {
public:
	ActionLed(Pin* gpio);
	virtual ~ActionLed();
	virtual void setPattern(const char* pattern);
	virtual String getHTMLCfg(String urlprefix);
	virtual String getHTMLController(String urlprefix);
	virtual void setSettings(String key, String value);
	virtual void setSettings(int status);
	virtual int loop();

private:
	Pin* gpio;
	int currentStatus = 0;
	LinkedList<int> pattern = LinkedList<int>();
	int patternPos = 0;

};

#endif /* ACTIONLED_H_ */
