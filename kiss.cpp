#include "Arduino.h"
#include "kiss.h"


Kiss::Kiss() {}
Kiss::~Kiss() {}



void Kiss::receiveFromHost(void)
{
  if(!kissTerm->available()){
    //logTerm->println("nothing to rx from host");
    state = KISS_STATE_IDLE;
    return;
  } 

  logTerm->println("Receiving KISS message");

  // Receive KISS message:
  while(kissTerm->available()){
    // read one byte:
    readByte = kissTerm->read();

    // check if it's a frame-end char (either opening or closing):
    if(readByte==KISS_FEND){
      // if not currently receiving then this must be opening FEND:
      if(state!=KISS_STATE_RX_FROM_HOST) {
        // start kiss frame capture:
        //logTerm->println("found KISS opening frame-end char");
        state = KISS_STATE_RX_FROM_HOST;
        kissFrameFromHostLen = 0;
        kissFrameFromHost[kissFrameFromHostLen++] = readByte;

      // if already receiving then this must be closing FEND:
      } else if(state==KISS_STATE_RX_FROM_HOST) {
        // close kiss frame:
        //logTerm->println("found KISS closing frame-end char");
        kissFrameFromHost[kissFrameFromHostLen++] = readByte;
        // if there's anything left in the buffer then dump it:
        if(kissTerm->available()){
          logTerm->println("Warning: found chars after closing frame-end");
          //dumpKissTerm();
          //return;
        }

        // Found end of KISS frame!  Process it:
        if(kissFrameFromHostLen>=3){
          printKissFrame();
          parseKissFrame();
        // if frame length is too short then complain:
        } else {
          logTerm->print("Error: KISS frame length: ");
          logTerm->println(kissFrameFromHostLen);
          printKissFrame();
        }
        state = KISS_STATE_IDLE;
        return;

      // otherwise, found FEND but not in expected KISS state
      // complain and dump the buffer:
      } else {
        logTerm->print("Error, found FEND char in state: ");
        logTerm->println(state);
        dumpKissTerm();
        return;
      }

    // if receiving then receive:
    } else if(state==KISS_STATE_RX_FROM_HOST && kissFrameFromHostLen<KISS_MAX_FRAME_LEN) {
      kissFrameFromHost[kissFrameFromHostLen++] = readByte;

    // if reached max allowable msg length then dump and reset buffer:
    } else if(kissFrameFromHostLen>=KISS_MAX_FRAME_LEN) {
      logTerm->println("Error:  reached max KISS frame length. Dumping buffer.");
      kissFrameFromHostLen = 0;
      dumpKissTerm();
      return;
    }
  }
  printKissFrame();
}


void Kiss::printKissFrame(){
  if(kissFrameFromHostLen>0){
    Serial.println("Received KISS frame:");
    for(int i=0; i<kissFrameFromHostLen; i++){
      Serial.print(" ");
      Serial.print(kissFrameFromHost[i],HEX);
    }
    Serial.println();
  }
}


void Kiss::parseKissFrame(){
  logTerm->println("Parsing KISS frame");
  // make sure first byte is the starting-fend:
  if(kissFrameFromHost[0]!=KISS_FEND){
    logTerm->print("Error, expected starting frame-end char: 0xC0, received: ");
    logTerm->println(kissFrameFromHost[0],HEX);
    //printKissFrame();
    return;
  }

  // read second byte and make sure it's valid KISS cmd:
  uint8_t kissCmd = kissFrameFromHost[1] & 0x0F;  // bitmask to see right nibble only
  switch(kissCmd){
    case  KISS_CMD_DATA_FRAME:
          getAX25Packet();
          break;

    case  KISS_CMD_TX_DELAY:
          setTxDelay();
          break;

    case  KISS_CMD_P:
          setPersistence();
          break;

    case  KISS_CMD_SLOT_TIME:
          setSlotTime();
          break;

    case  KISS_CMD_TX_TAIL:
          setTxTail();
          break;

    case  KISS_CMD_FULL_DUPLEX:
          setFullDuplex();
          break;

    case  KISS_CMD_SET_HARDWARE:
          setHardware();
          break;

    case  KISS_CMD_RETURN:
          endKiss();
          break;

    default:
          logTerm->print("Error, unrecognized KISS cmd: ");
          logTerm->println(kissCmd,HEX);
          dumpKissTerm();
          return;
  }
}


void Kiss::getAX25Packet(){
  logTerm->println("Extracting AX25 packet from KISS frame");
  // Extract AX25 Packet from KISS "info":
  ax25OutgoingPacketLen = 0;
  // start at 3rd byte since should have previously verified FEND
  // and KISS cmd in first 2 bytes
  for(int i=2; i<kissFrameFromHostLen-1; i++){
    // Handle KISS escaped chars:
    // The FEND code is then sent as FESC, TFEND
    //  => If this char is TFEND and prev was FESC then
    //     overwrite prev entry (FESC) in AX25 with FEND
    // and the FESC is then sent as FESC, TFESC.
    //  => If this char is TFESC and prev was FESC then
    //     overwrite prev entry (FESC) in AX25 with FESC
    if(kissFrameFromHost[i-1]==KISS_FESC && kissFrameFromHost[i]==KISS_TFEND){
      ax25OutgoingPacket[ax25OutgoingPacketLen] = KISS_FEND;
      
    } else if(kissFrameFromHost[i-1]==KISS_FESC && kissFrameFromHost[i]==KISS_TFESC){
      ax25OutgoingPacket[ax25OutgoingPacketLen] = KISS_FESC;

    // otherwise append to ax25 packet:
    } else if(ax25OutgoingPacketLen<AX25_MAX_FRAME_LEN){
      ax25OutgoingPacket[ax25OutgoingPacketLen++] = kissFrameFromHost[i];
    }
  }

  // if total length exceeds buffer length then complain and quit:
  if(ax25OutgoingPacketLen>AX25_MAX_FRAME_LEN){
    logTerm->println("Error: AX25 packet from KISS host exceeds max length");
    logTerm->print("Extracted AX25 packet length: ");
    logTerm->println(ax25OutgoingPacketLen);  
    return;
  } 

  // do something with the received packet:
  printOutgoingAX25Packet();
  // TO-DO:  send ax25 packet to modulator
  Serial.println("Sending packet to modulator (not really)");
  Serial.println("..done");
}



void Kiss::printOutgoingAX25Packet(){
  Serial.println("Outgoing AX25 packet:");
  for(int i=0; i<ax25OutgoingPacketLen; i++){
    Serial.print(" ");
    Serial.print(ax25OutgoingPacket[i],HEX);
  }
  Serial.println();
}



void Kiss::setTxDelay(){
  logTerm->println("received SET TX DELAY instruction.");
  configTxDelay = 10 * (uint16_t)kissFrameFromHost[2];
  logTerm->printf("Set TX delay to %i ms\n", configTxDelay);
}

void Kiss::setPersistence(){
  logTerm->println("received SET PERSISTENCE instruction.");
  configPersistence = (uint16_t)kissFrameFromHost[2];
  logTerm->printf("Set persistence to %i\n", configPersistence);  
}

void Kiss::setSlotTime(){
  logTerm->println("received SET SLOT TIME instruction.");
  configSlotTime = 10 * (uint16_t)kissFrameFromHost[2];
  logTerm->printf("Set slot time to %i ms\n", configSlotTime);
}

void Kiss::setTxTail(){
  logTerm->println("received SET TX TAIL instruction.");
  configTxTail = 10 * (uint16_t)kissFrameFromHost[2];
  logTerm->printf("Set TX tail to %i ms\n", configTxTail);
}

void Kiss::setFullDuplex(){
  logTerm->println("received SET DUPLEX instruction.");
  configDuplex = (uint8_t)kissFrameFromHost[2];
  if(configDuplex==HALF_DUPLEX)
    logTerm->println("Setting duplex to HALF duplex.");
  else
    logTerm->println("Setting duplex to FULL duplex.");
}

void Kiss::setHardware(){
  logTerm->println("received SET HARDWARE instruction (not implemented).");
}

void Kiss::endKiss(){
  logTerm->println("received RETURN instruction.");
}



void Kiss::closeKissFrame(){
  readByte = kissTerm->read();
  if(readByte!=KISS_FEND){
    logTerm->print("Error: expected closing frame-end char, received: ");
    logTerm->println(readByte,HEX);
  }
}


void Kiss::dumpKissTerm(){
  logTerm->println("Rest of kiss term buffer:");
  echoFromHostToLog();
  state = KISS_STATE_IDLE;
}





void Kiss::echoFromHostToLog(void) 
{
  unsigned char readByte;

  while (kissTerm->available()) 
  {
    readByte = kissTerm->read();
    // if kiss wasn't previously receiving then this is a new message
    // make sure the packet starts with start-frame FEND
    if(state!=KISS_STATE_RX_FROM_HOST){
      if (readByte == KISS_FEND){
        logTerm->print("New frame: ");
        logTerm->printf("%02X",readByte);
        state = KISS_STATE_RX_FROM_HOST;
        kissFrameFromHost[0] = readByte;
        kissFrameFromHostLen=1;
      } else {
        // uh oh, expected start-frame FEND
        logTerm->println("Error, expected frame to start with 0xC0");
      }
    } 
    else {
      if (readByte != KISS_FEND && kissFrameFromHostLen<KISS_MAX_FRAME_LEN) {
        logTerm->printf("%02X",readByte);
        logTerm->print(" ");
        kissFrameFromHost[kissFrameFromHostLen++];
      } else {
        // found end of frame
        state = KISS_STATE_IDLE;
        logTerm->printf("%02X",readByte);
        logTerm->println(" /end frame");
      }
    }
  }
}