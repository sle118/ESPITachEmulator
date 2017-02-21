#include <Service.h>

namespace iTach {
Service::Service(int udpPort, int port, int irSendPin, int recvPin, int debugPort,
		const String& debugMessage) :
		IrServiceBase(port, irSendPin, recvPin, debugPort, debugMessage), ipMulti(
				239, 255, 250, 250) {
	learning = false;

	uint8_t mac[6];
	WiFi.macAddress(mac);
	clientName = macToStr(mac);
	clientName.replace(":", "");
	portMulti = udpPort;
	beaconInterval = random(10000, 60000); // Send every 10...60 seconds like the original
										   // const long interval = 3000; // for debugging
	currentLearner = NULL;
	nextBeaconTime = 0;
}
Service::~Service() {
	Udp.flush();

}
void Service::OnBegin() {
	registerEvents([this](decode_results * decode) {
		if (this->learning)
		{
			if( currentLearner!=NULL )
			{
				unsigned long freq = 38400;        // TODO: FIXME DON'T HARDCODE
				stringDecode = "sendir,1:0,0,";
				stringDecode += freq;
				stringDecode += (",1,1,");
				this->dump(decode);// append the raw sequences to the message
				stringDecode += "999";
				if(decode->rawlen % 2 ==0)
				{
					debugSend("Invalid length "+String(decode->rawlen)+" for IR signal : "+stringDecode);
				}
				else
				{
					this->send(*this->currentLearner, stringDecode);
					this->learning = false;// The iTach specs says we need to stop the learning mode at this point.
				}
			}
			else
			{
				debugSend("iTach IR Event received in learning mode, but current learner is null!");
			}
		}

		return true;
	});
	//		getdevices
	//		Sent from each iTach module in response to getdevices:
	//		device,<moduleaddress>,<moduletype> (one sent for each module)
	//		where for iTach products;
	//		<moduleaddress> is |0|1|
	//		<moduletype> is |WIFI|ETHERNET|3 RELAY|3 IR|1 SERIAL|
	registerCommand("getdevices",
			[this](WiFiClient& client, const String& args) {
				send(client, "device,0,0 WIFI");
				send(client, "device,1,3 IR");
				send(client, "endlistdevices");
				return true;
			});
	registerCommand("get_IRL", [this](WiFiClient& client, const String& args) {
		learning = true;
		currentLearner = &client;
		bool found=false;
		send(client, "IR Learner Enabled");
		return true;
	});

	registerCommand("stop_IRL", [this](WiFiClient& client, const String& args) {
		send(client, "IR Learner Disabled");
		learning = false;
		return true;
	});
	//		get_NET
	//		This command will retrieve the current network settings and return a comma delimited string with the
	//		network settings.
	//		Sent to iTach:
	//		get_NET,0:1↵
	//		Sent from iTach in response to get_NET command:
	//		NET,0:1,<configlock>,<ipsettings>,<ipaddress>,<subnet>,<gateway>
	//		where;
	//		<configlock> |LOCKED|UNLOCKED|
	//		<ipsettings> |DHCP|STATIC|
	//		<ipaddress> is the assigned network IP
	//		<subnet> is the network subnet mask
	//		<gateway> is the default network gateway
	registerCommand("get_NET",
			[this](WiFiClient& client, const String& args) {
				bool bAddressFound=false;
				String connectoraddress = this->getNthToken(1,args,&bAddressFound);
				send(client, "NET,"+connectoraddress+",LOCKED,"+WiFi.localIP().toString()+","+WiFi.subnetMask().toString()+","+WiFi.gatewayIP().toString());
				return true;
			});


	registerCommand("getversion",
			[this](WiFiClient& client, const String& args) {
				send(client, "1.0");
				return true;
			});

	registerCommand("sendir", 8, [this](WiFiClient& client, const String& args) {
		return this->onSendIr(client,args);
	} );
}
bool Service::onSendIr(WiFiClient& client, const String& args)
{
	iTachAsyncIRCallback * callbackData = new iTachAsyncIRCallback(client);
//		sendir,<connectoraddress>,<ID>,<frequency>,<repeat>,<offset>,<on1>,
//		<off1>,<on2>,<off2>,….,<onN>,<offN> (where N is less than 260 or a total of 520 ↵
//		numbers)
//		where;
//		<connectoraddress> is as defined in section 1.
//		<ID> is |0|1|2|…|65535| (1) (for the completeir response, see below)
//		<frequency> is |15000|15001|….|500000| (in hertz)
//		<repeat> is |1|2|….|50| (2) (the IR command is sent <repeat> times)
//		<offset> is |1|3|5|….|383| (3) (used if <repeat> is greater than 1, see below)
//		<on1> is |1|2|…|65635| (4) (number of pulses)
//		<off1> is |1|2|…|65635| (4) (absence of pulse periods of the carrier frequency)
	//		1   2 3     4 5
	// 		1:1,4,38400,1,69,347,173,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,65,22,65,22,65,22,65,22,65,22,65,22,65,22,65,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,65,22,65,22,65,22,65,22,65,22,65,22,65,22,65,22,1527,347,87,22,3692");
	DEBUG_PRINT("Getting number of raw values from command");
	debugSend("Queuing IR transmission");

	int numberOfPulses = getNumberOfRawValues(args, 6);
	numberOfPulses = 48;
	if(numberOfPulses<=0)
	{
		sendError(client, ERR_Bad_command_syntax);
		return true;
	}
	long position=0;

	DEBUG_PRINTF("Number of pulses was %3d. getting connector address.\n",numberOfPulses);
	callbackData->connectoraddress = this->getNextToken(args,&position);
	debugSend("address was "+callbackData->connectoraddress+" getting ID ");
	callbackData->ID = this->getNextToken(args,&position);


	DEBUG_PRINTF("id was [%s]. Setting up command processor\n",callbackData->ID.c_str() );

	IrAsyncCommandProcessor * command = new IrAsyncCommandProcessor((void *) callbackData, numberOfPulses);
	//IrAsyncCommandProcessor command((void *) callbackData, numberOfPulses);
//
	DEBUG_PRINT("done setting command processor. Getting frequency.\n");
	command->frequency = this->getNextToken(args,&position).toInt();
	DEBUG_PRINTF("Frequency was %3d. Adjusting.\n",command->frequency);
	command->frequency=command->frequency>0?command->frequency:38000;
	command->frequencyKhz = (command->frequency/1000)+ 0.5;
	DEBUG_PRINTF("Frequency now %3d, khz is %3d. Getting repeat count.\n",command->frequency, command->frequencyKhz);
	command->repeat = this->getNextToken(args,&position).toInt();
	DEBUG_PRINTF("Repeat is %3d. Getting offset.\n", command->repeat);
	 //set the offset with 0 being the first element of the repeated pattern
	command->offset = this->getNextToken(args,&position).toInt()-1;
	DEBUG_PRINTF("offset is %3d. Computing unique ID.\n", command->offset);
//
	if(command->repeat>50)
	{
		sendError(client, ERR_Repeat_count_exceeded);
		debugSend("repeat>0");
		return true;
	}
	if(command->offset<0)
	{
		sendError(client, ERR_Bad_command_syntax);
		debugSend("Offset<0");
		return true;
	}
	if(numberOfPulses % 2 > 0 )
	{
		sendError(client, ERR_Uneven_amount_of_on_off_statements);
		return true;
	}
	command->uniqueID = "itach"+callbackData->ID;
	DEBUG_PRINTF("uniqueID is %s. Initializing parser. Values:\n", command->uniqueID.c_str());
	iTach::IrParser parser(position, args);
	unsigned int * val = command->buf;
	size_t currentElement=0;
	while(parser.getNext())
	{
		if(currentElement<numberOfPulses)
		{
			DEBUG_PRINTF_NO_HEADER("%03d,%03d,", parser.value1,parser.value2);
			command->buf[currentElement] = ((1000000 / command->frequency) * parser.value1) + 0.5;
			currentElement++;
			val++;
			*val = ((1000000 / command->frequency) * parser.value2) + 0.5;
			currentElement++;
			val++;
		}
	}
	DEBUG_PRINTF_NO_HEADER(" total %03d entries.\n Setting up callback.\n", parser.totalNumberOfValues);
	debugSend("Compressed form is : "+parser.getCompressedString());
	debugSend("Setting up Callback");
	command->onComplete([this](void * callbackData) {
				iTachAsyncIRCallback * data = (iTachAsyncIRCallback * )callbackData;
				debugSend("Completed IR callback.\n");
				if(data!=NULL)
				{
					this->send(data->client, "completeir,"+data->connectoraddress+","+data->ID);
					delete data;
					return true;
				}
				else
					{
						debugSend("Callback Data was null.\n");
						return false;
					}

			});
	DEBUG_PRINT("Callback setup, queuing the command.\n");
	this->addAsyncIRCommand(command);
	debugSend("Done queuing IR transmission");
	return true;
}

size_t Service::getNumberOfRawValues(const String& data, int startElement)
{
	int numberOfValues = 0;        //variable to count data part nr
	int totalNumberOfValues = 1;
	String dictList;
	const char *p = data.c_str();
	DEBUG_PRINTF("<%02d.",numberOfValues);

	while(*p !='\0') { //Walk through the text one letter at a time
		if (*p == ',') {
			totalNumberOfValues++;
			if(totalNumberOfValues>=startElement)
			{
				numberOfValues++;
				DEBUG_PRINTF_NO_HEADER("%02d.",numberOfValues);
			}
		}
		else if(isAlpha(*p))
		{
			numberOfValues+=2;
			DEBUG_PRINTF_NO_HEADER("%02dvv",numberOfValues);
		}
		p++;
	}

	DEBUG_PRINTF_NO_HEADER("> total: %d\n",numberOfValues);
	numberOfValues=numberOfValues>0?numberOfValues:0;
	return numberOfValues;
}

void Service::sendError(WiFiClient& client, ERROR_CODES errcode) {
		send(client,String("unknowncommand,")+ (errcode < 10 ? "0" : "")+ String(errcode));
}
void Service::OnProcess() {
	if (millis() >= nextBeaconTime) {
		sendDiscoveryBeacon();
	}
	if (!timeIsOut()) {
		// do  some other stuff
	}
}


void Service::OnUnknownCommand(String& inData, WiFiClient& client) {
	sendError(client, ERR_Invalid_command_Command_not_found);
}



// The Discovery Beacon is a UDP packet sent to
// the multicast IP address 239.255.250.250 on UDP port number 9131
// To check its correctness, use the AmxBeaconListener that is built into IrScrutinizer
// The Beacon message must include Device-SDKClass, Device-Make, and Device-Model.
// For IP controlled devices Device-UUID is added for identification purposes.
void Service::sendDiscoveryBeacon() {

	String buffer = "AMXB<-UUID=WF2IR_";
	buffer += clientName;
	buffer +=
			"><-SDKClass=Utility><-Make=GlobalCache><-Model=WF2IR><-Config-URL=http://";
	buffer += WiFi.localIP().toString();
	buffer += "><-Status=Ready>\r";
	Udp.beginPacket(ipMulti, portMulti);
	Udp.print(buffer);
	// AMX beacons needs to be terminated by a carriage return (�\r�, 0x0D)
	Udp.endPacket();
	nextBeaconTime = millis() + beaconInterval;
	debugSend("Sent beacon ", true);
	debugSend(buffer);
}

// Convert the MAC address to a String
// This kind of stuff is why I dislike C. Taken from https://gist.github.com/igrr/7f7e7973366fc01d6393
String Service::macToStr(const uint8_t* mac) {
	String result;
	for (int i = 0; i < 6; ++i) {
		result += String(mac[i], 16);
		if (i < 5)
			result += ':';
	}
	return result;
}
}
