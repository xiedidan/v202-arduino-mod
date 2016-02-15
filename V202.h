#include "Arduino.h"

class nRF24;
class V202_TX {
  nRF24& radio;
  uint8_t txid[3];
  uint8_t rf_channels[16];
  bool packet_sent;
  uint8_t rf_ch_num;
public:
  V202_TX(nRF24& radio_) :
    radio(radio_)
  {}
  void setTXId(uint8_t txid_[3]);
  void begin();
  void command(uint8_t throttle, int8_t yaw, int8_t pitch, int8_t roll, uint8_t flags);
};

