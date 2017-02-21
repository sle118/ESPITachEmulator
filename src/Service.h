/*
 * iTach.h
 *
 *  Created on: 8 f�vr. 2017
 *      Author: Sebastien
 */

#ifndef SRC_SERVICE_H_
#define SRC_SERVICE_H_
#include <Arduino.h>
#include <IrParser.h>
#include <IrServiceBase.h>
#include "WiFiUdp.h"
#include "ESP8266WiFi.h"
#include "functional"
#include "StringArray.h"
namespace iTach {
enum ERROR_CODES
{
	ERR_Invalid_command_Command_not_found,
	ERR_Invalid_module_address_does_not_exist,
	ERR_Invalid_connector_address_does_not_exist,
	ERR_Invalid_ID_value,
	ERR_Invalid_frequency_value,
	ERR_Invalid_repeat_value,
	ERR_Invalid_offset_value,
	ERR_Invalid_pulse_count,
	ERR_Invalid_pulse_data,
	ERR_Uneven_amount_of_on_off_statements,
	ERR_No_carriage_return_found,
	ERR_Repeat_count_exceeded,
	ERR_IR_command_sent_to_input_connector,
	ERR_Blaster_command_sent_to_non_blaster_connector,
	ERR_No_carriage_return_before_buffer_full,
	ERR_No_carriage_return,
	ERR_Bad_command_syntax,
	ERR_Sensor_command_sent_to_non_input_connector,
	ERR_Repeated_IR_transmission_failure,
	ERR_Above_designated_IR_on_off_pair_limit,
	ERR_Symbol_odd_boundary,
	ERR_Undefined_symbol,
	ERR_Unknown_option,
	ERR_Invalid_baud_rate_setting,
	ERR_Invalid_flow_control_setting,
	ERR_Invalid_parity_setting,
	ERR_Settings_are_locked
};


class Service : public IrServiceBase
{
public:

	Service(int udpPort, int port, int irSendPin, int recvPin, int debugPort, const String& debugMessage="");
	~Service();

private:
	WiFiUDP Udp;
	String clientName ;
	IPAddress ipMulti;
	time_t nextBeaconTime;
	WiFiClient* currentLearner;
	bool learning;
	unsigned int portMulti;
	unsigned int beaconInterval;
	virtual void OnBegin();
	virtual void OnProcess();
	virtual void OnUnknownCommand(String& inData,WiFiClient& client);
	void sendError(WiFiClient& client, ERROR_CODES errcode);
	bool onSendIr(WiFiClient& client, const String& args);
	void sendDiscoveryBeacon();
	String macToStr(const uint8_t* mac);
	size_t getNumberOfRawValues(const String& data, int startElement);

};
class iTachAsyncIRCallback
{
public:
	iTachAsyncIRCallback(WiFiClient& request_client): connectoraddress(""),ID(""), client(request_client){}
	WiFiClient& client ;
	String connectoraddress;
	String ID;
};

}
#endif /* SRC_SERVICE_H_ */
