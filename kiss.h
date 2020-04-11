#ifndef kiss_h
#define kiss_h

#include "Arduino.h"


#define AX25_MAX_FRAME_LEN  2048 // bytes
#define KISS_MAX_FRAME_LEN  2051 // bytes

#define KISS_FEND   0xC0
#define KISS_FESC   0xDB
#define KISS_TFEND  0xDC
#define KISS_TFESC  0xDD

#define KISS_CMD_DATA_FRAME   0x00  // Data from TNC to host
#define KISS_CMD_TX_DELAY     0x01  // Delay defined in (1-byte N) x 10ms
#define KISS_CMD_P            0x02  // Persistence = Data*256-1, used for CSMA
#define KISS_CMD_SLOT_TIME    0x03  // Slot time in (1-byte N) * 10 ms
#define KISS_CMD_TX_TAIL      0x04  // Time to keep transmitter keyed, in 10ms units
#define KISS_CMD_FULL_DUPLEX  0x05  // 0=half, else=full
#define KISS_CMD_SET_HARDWARE 0x06  // device dependent
#define KISS_CMD_RETURN       0xFF  // Exit KISS; requires port code 0x0F

using namespace std;


enum KissState {
  KISS_STATE_IDLE,
  KISS_STATE_RX_FROM_HOST,
  KISS_STATE_TX_TO_HOST,
  KISS_STATE_RX_FROM_RADIO,
  KISS_STATE_TX_TO_RADIO
};

enum Duplex {
    HALF_DUPLEX,
    FULL_DUPLEX
};


class Kiss {
  public:
    Kiss();
    ~Kiss();

    unsigned char readByte;

    KissState state;

    Stream *kissTerm;
    Stream *logTerm;

    uint16_t configTxDelay = 500; //msec
    uint8_t  configPersistence = 63; 
    uint16_t configSlotTime = 100; //msec
    uint16_t configTxTail = 0; //msec
    uint8_t  configDuplex = HALF_DUPLEX;


    unsigned char kissFrameFromHost[KISS_MAX_FRAME_LEN];
    uint16_t kissFrameFromHostLen = 0;

    unsigned char kissFrameToHost[KISS_MAX_FRAME_LEN];
    uint16_t kissFrameToHostLen = 0;

    unsigned char ax25OutgoingPacket[AX25_MAX_FRAME_LEN];
    uint16_t ax25OutgoingPacketLen = 0;

    unsigned char ax25IncomingPacket[AX25_MAX_FRAME_LEN];
    uint16_t ax25IncomingPacketLen = 0;

    void setKissTerminal(Stream *streamObj) { kissTerm = streamObj; }
    void setLogTerminal(Stream *streamObj) { logTerm = streamObj; }

    void receiveFromHost();
    void parseKissFrame();
    void printKissFrame();
    void closeKissFrame();
    void dumpKissTerm();
    void echoFromHostToLog();

    void getAX25Packet();
    void setTxDelay();
    void setPersistence();
    void setSlotTime();
    void setTxTail();
    void setFullDuplex();
    void setHardware();
    void endKiss();

    void printOutgoingAX25Packet();
  
};

#endif