/*
 * Webserver.cpp
 *
 *  Created on: 20.08.2016
 *      Author: sven
 */

#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <Arduino.h>
#include <FS.h>

#include "Webserver.h"
#include "Controller.h"
#include "Logger.h"
#include "WebserviceLog.h"
#include "Utils.h"

ESP8266WebServer* Webserver::server = 0;
ESP8266HTTPUpdateServer* Webserver::httpUpdater = 0;


Webserver::Webserver(Controller* c) {
	controll = c;

	server = new ESP8266WebServer(80);
	server->onNotFound(std::bind(&Webserver::handleNotFound, this));
	server->on("/", std::bind(&Webserver::handleRoot, this));
	server->on("/version", std::bind(&Webserver::handleVersion, this));
	server->on("/controll", std::bind(&Webserver::handleController, this));
	server->on("/cfg", std::bind(&Webserver::handleCfg, this));
	server->on("/set", std::bind(&Webserver::handleSet, this));
	server->on("/list", std::bind(&Webserver::handleFilelist, this));
	server->on("/upload", HTTP_POST, []() { server->send(200, "text/html", "<meta http-equiv=\"refresh\" content=\"1; URL=/list\">"); }, std::bind(&Webserver::handleUpload, this));
	server->on("/del", std::bind(&Webserver::handleDel, this));
	server->begin();
	httpUpdater = new ESP8266HTTPUpdateServer();
	const char* update_path = "/firmware";
	httpUpdater->setup(server, update_path, "admin", "admin");

}

void Webserver::handleUpload() {
	HTTPUpload& upload = server->upload();
	if(upload.status == UPLOAD_FILE_START){
		String filename = upload.filename;
		if(!filename.startsWith("/")) {
			filename = "/"+filename;
		}
		fsUploadFile = SPIFFS.open(filename, "w");
		filename = String();
	} else if(upload.status == UPLOAD_FILE_WRITE) {
		if(fsUploadFile) {
			fsUploadFile.write(upload.buf, upload.currentSize);
		}
	} else if(upload.status == UPLOAD_FILE_END){
		if(fsUploadFile) {
			fsUploadFile.close();
		}
	}
	yield();
}

void Webserver::handleFilelist() {
	Dir dir = SPIFFS.openDir("/");

	String output = "" + Utils::getHTMLHeader() + F("<table><thead><tr><th>Name</th><th>Size</th></thead><tbody>");

	while(dir.next()){
		File entry = dir.openFile("r");
		output += "<tr><td>";
		output += "<a href=\"" + dir.fileName() + "\">";
		output += String(dir.fileName()).substring(1);
		output += "</a>";
		output += "</td><td>";
		output += String(dir.fileSize());
		output += "</td><td>";
		output += "<a href=\"/del?file=" + dir.fileName() + "\">";
		output += "&#x2421";
		output += "</a>";
		output += "</td></tr>";
	}
	output += F("</tbody></table><hr>");

	output += F("<form action=\"/upload\" method=\"post\" enctype=\"multipart/form-data\"><fieldset> <input name=\"Datei\" type=\"file\" size=\"50\"> ");
	output += F("    <input class=\"button-primary\" value=\"Send\" type=\"submit\"> </fieldset></form> ");
	output += Utils::getHTMLFooter();
	server->send(200, "text/html", output);
}

void Webserver::handleRoot() {
	String message = Utils::getHTMLHeader();
	message += "<div class=\"row\">"

			"<div class=\"column\">"
			"<a style=\"font-size: 4rem;\" class=\"button\" href=\"/list\">&#128194;</a>"
			"</div>";

	for (int i = 0; i < services.size(); i++) {
		WebserviceBase* s = services.get(i);
		String linktext = s->getLinkText();
		if (linktext.equals("")) {
			continue;
		}
		message += "<div class=\"column\">";
		message += "<a style=\"font-size: 4rem;\" class=\"button\" href=\"" + String(s->getUri()) + "\">" + linktext + "</a>";
		message += "</div>";
	}

	message += "</div>";
	message += Utils::getHTMLFooter();
	server->send(200, "text/html", message);

}



void Webserver::handleVersion() {
	server->send(200, "text", compile_date);

}

int Webserver::loop() {
	server->handleClient();

	if (lastWifiStatus != WiFi.status()) {
		Logger::getInstance()->addToLog(
				"Wifi status changed: " + Utils::wifi2String(lastWifiStatus)
		+ " => " + Utils::wifi2String(WiFi.status()) + " IP:"
		+ WiFi.localIP().toString());
		Serial.printf("Connection to: %s (Q:%d)\r\n", WiFi.BSSIDstr().c_str(), WiFi.RSSI());
		lastWifiStatus = WiFi.status();
		if (WiFi.status() == WL_CONNECTED) {
			MDNS.begin(controll->getHostname().c_str());
			MDNS.addService("http", "tcp", 80);
		}
	}
	return 2;

}

Webserver::~Webserver() {
}

/**
 * Liefert eine Datei aus dem SPIFFS-Filesystem.
 * Funktioniert zur Zeit noch nicht
 */
bool Webserver::loadFromSPIFFS(String path) {
	Serial.println("Searching for " + path);
	String dataType = "text/plain";
	if (path.endsWith("/"))
		path += "index.htm";
	if (!SPIFFS.exists(path)) {
		return false;
	}
	if (path.endsWith(".src"))
		path = path.substring(0, path.lastIndexOf("."));
	else if (path.endsWith(".htm"))
		dataType = "text/html";
	else if (path.endsWith(".html"))
		dataType = "text/html";
	else if (path.endsWith(".txt"))
		dataType = "text/plain";
	else if (path.endsWith(".css"))
		dataType = "text/css";
	else if (path.endsWith(".js"))
		dataType = "application/javascript";
	else if (path.endsWith(".png"))
		dataType = "image/png";
	else if (path.endsWith(".gif"))
		dataType = "image/gif";
	else if (path.endsWith(".jpg"))
		dataType = "image/jpeg";
	else if (path.endsWith(".ico"))
		dataType = "image/x-icon";
	else if (path.endsWith(".xml"))
		dataType = "text/xml";
	else if (path.endsWith(".pdf"))
		dataType = "application/pdf";
	else if (path.endsWith(".zip"))
		dataType = "application/zip";
	File dataFile = SPIFFS.open(path.c_str(), "r");
	int transmit = server->streamFile(dataFile, dataType);
	Serial.println(
			"Transmit: " + String(transmit) + " Size: "
			+ String(dataFile.size()));
	dataFile.close();
	return true;
}

void Webserver::handleNotFound() {
	if (loadFromSPIFFS(server->uri())) {
		return;
	}
	if (server->hostHeader().equals(server->client().localIP().toString())) {
			String message = "File Not Found\n\n";
			message += "URI: ";
			message += server->uri();
			message += "\nMethod: ";
			message += (server->method() == HTTP_GET) ? "GET" : "POST";
			message += "\nArguments: ";
			message += server->args();
			message += "\n";
			for (uint8_t i = 0; i < server->args(); i++) {
				message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
			}
			server->send(404, "text/plain", message);
	} else {
		Serial.println(server->hostHeader());
		server->sendHeader("Location", String("http://") + server->client().localIP().toString(), true);
		server->send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
		server->client().stop(); // Stop
	}
}


void Webserver::handleController() {
	String message = Utils::getHTMLHeader();
	message += controll->getHTMLController();
	message += Utils::getHTMLFooter();
	server->send(200, "text/html", message);
}

void Webserver::registerWebServices(WebserviceBase* base) {
	services.add(base);
	base->setServer(server);
	server->on(base->getUri(), std::bind(&WebserviceBase::run, base));
}

void Webserver::handleDel() {
	SPIFFS.remove(server->arg("file"));
	server->send(200, "text/html", "<html><head><META http-equiv=\"refresh\" content=\"1;URL=/list\"></head><body>Deleting...</body></html>");
}
void Webserver::handleSet() {
	Serial.println("Webserver");
	controll->setRequest(server->arg("id"), server->arg("key"), server->arg("value"));
	server->send(200, "text/html", "<html><head><META http-equiv=\"refresh\" content=\"1;URL=/controll\"></head><body>Sending...</body></html>");
}

void Webserver::handleCfg() {
	String message = Utils::getHTMLHeader();
	message += controll->getHTMLCfg();
	message += Utils::getHTMLFooter();
	server->send(200, "text/html", message);
}
