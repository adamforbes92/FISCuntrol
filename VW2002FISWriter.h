#ifndef VW2002FISWriter_h
#define VW2002FISWriter_h

#include <inttypes.h>
#include <Arduino.h>

// based on FIS_emulator
#define FIS_WRITE_PULSEW 4
#define FIS_WRITE_START 15 //something like address, first byte is always 15

#define PORT_3LB PORTB
#define DATA  PB3 //MOSI (Arduino Uno 11)
#define CLK   PB5 //SCK (Arduino 13)

// Positions in Message-Array
#define FIS_MSG_COMMAND 0
#define FIS_MSG_LENGTH 1

static uint8_t off[] = {0x81, 18, 240, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 98};
static uint8_t blank[] = {0x81, 18, 240, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 98};

class VW2002FISWriter
{
  public:
    VW2002FISWriter(uint8_t clkPin, uint8_t dataPin, uint8_t enaPin);
    ~VW2002FISWriter();
    void FIS_init();
    void init_graphic();
    void remove_graphic();
    void write_text_full(int x, int y, String line);
    void write_graph(String line);

    void sendMsg(String line1, String line2, bool center = true);
    void sendMsg(char msg[]);
    void sendRawMsg(char in_msg[]);
    void sendKeepAliveMsg();
    void displayOff();
    void displayBlank();
    void printFreeMem();

  private:
    uint8_t _FIS_WRITE_CLK;
    uint8_t _FIS_WRITE_DATA;
    uint8_t _FIS_WRITE_ENA;

    void FIS_WRITE_send_3LB_msg(char in_msg[]);
    void FIS_WRITE_send_3LB_singleByteCommand(uint8_t txByte);
    void sendEnablePulse();
    void FIS_WRITE_3LB_sendByte(int in_byte);
    void FIS_WRITE_startENA();
    void FIS_WRITE_stopENA();
    void setClockHigh();
    void setClockLow();
    void setDataHigh();
    void setDataLow();
    uint8_t checksum( volatile uint8_t in_msg[]);
};

#endif

