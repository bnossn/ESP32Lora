/*
  Slave.h - Library for defining Slave class.
  Created by Bruno N., January 2, 2018.
*/
#ifndef Slave_h
#define Slave_h

#include "Arduino.h"

class Slave
{
  public:	
	Slave();
	Slave(uint64_t id);
	uint64_t id;
	uint16_t DigIn;
	uint16_t DigOut;
	bool DigOutmodified; //indica para o master se houve alteracao pelo usuario
	uint16_t AO[2];
	bool AOmodified[2]; //indica para o master se houve alteracao pelo usuario
	uint16_t AI[6];
	bool bmodSent; //Usado para controlar se a modificacao solicitada foi enviada ao Slave
	bool isOnline;
	uint16_t imsgCounter; //Usado para evitar ataques tipo repeater. Toda nova mensagem deve acrescer imsgCounter.
	bool bRstCounter;
	//long lastSentTime;
	long lastRecTime;
	int packetRssi;
	int packetSnr;
	byte getDO(int n); //Get value of DO n (0 or 1)
	byte getDI(int n); //Get value of DI n (0 or 1)
	void setDO(int n); 
	void resetDO(int n);
	
  private:
    
};

#endif