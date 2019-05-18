
#include "Arduino.h"
#include "Slave.h"

Slave::Slave()
{
	id = 0;
	DigIn = 0;
	DigOut = 0;
	for (int j = 0; j < (sizeof(AO)/sizeof(AO[0])); j++){
		AO[j] = 0;
	}
	for (int j = 0; j < (sizeof(AI)/sizeof(AI[0])); j++){
		AI[j] = 0;
	}
	isOnline = false;
	//lastSentTime = 0;
	imsgCounter = 0;
	bRstCounter = true; //sempre reseta quando inicia gtw
	lastRecTime = 0;
	DigOutmodified = false;
	for (int j = 0; j < (sizeof(AOmodified)/sizeof(AOmodified[0])); j++){
		AOmodified[j] = false;
	}
	bmodSent = true;
	
	packetRssi = 0;
	packetSnr = 0;
}

Slave::Slave(uint64_t i)
{
	id = i;
	DigIn = 0;
	DigOut = 0;
	for (int j = 0; j < (sizeof(AO)/sizeof(AO[0])); j++){
		AO[j] = 0;
	}
	for (int j = 0; j < (sizeof(AI)/sizeof(AI[0])); j++){
		AI[j] = 0;

	}
	isOnline = false;
	//lastSentTime = 0;
	imsgCounter = 0;
	bRstCounter = true; //sempre reseta quando inicia gtw
	lastRecTime = 0;
	DigOutmodified = false;
	for (int j = 0; j < (sizeof(AOmodified)/sizeof(AOmodified[0])); j++){
		AOmodified[j] = false;
	}
	bmodSent = true;
	
	packetRssi = 0;
	packetSnr = 0;
}

byte Slave::getDO(int n)
{
	return ((this->DigOut >> n) & 1);
}

byte Slave::getDI(int n)
{
	return ((this->DigIn >> n) & 1);
}

void Slave::setDO(int n)
{
	uint16_t i = 1;
	
	this->DigOut = this->DigOut | (i << n);
}

void Slave::resetDO(int n)
{
	uint16_t i = 1;
	
	this->DigOut = this->DigOut & ~(i << n); //= (DigOut and not(i << n))
	
}
