#include "V202.h"
#include "nRF24L01.h"

// This is frequency hopping table for V202 protocol
// The table is the first 4 rows of 32 frequency hopping
// patterns, all other rows are derived from the first 4.
// For some reason the protocol avoids channels, dividing
// by 16 and replaces them by subtracting 3 from the channel
// number in this case.
// The pattern is defined by 5 least significant bits of
// sum of 3 bytes comprising TX id
uint8_t freq_hopping[][16] = {
 { 0x27, 0x1B, 0x39, 0x28, 0x24, 0x22, 0x2E, 0x36,
   0x19, 0x21, 0x29, 0x14, 0x1E, 0x12, 0x2D, 0x18 }, //  00
 { 0x2E, 0x33, 0x25, 0x38, 0x19, 0x12, 0x18, 0x16,
   0x2A, 0x1C, 0x1F, 0x37, 0x2F, 0x23, 0x34, 0x10 }, //  01
 { 0x11, 0x1A, 0x35, 0x24, 0x28, 0x18, 0x25, 0x2A,
   0x32, 0x2C, 0x14, 0x27, 0x36, 0x34, 0x1C, 0x17 }, //  02
 { 0x22, 0x27, 0x17, 0x39, 0x34, 0x28, 0x2B, 0x1D,
   0x18, 0x2A, 0x21, 0x38, 0x10, 0x26, 0x20, 0x1F }  //  03
};

void V202_TX::setTXId(uint8_t txid_[3])
{
  uint8_t sum;
  txid[0] = txid_[0];
  txid[1] = txid_[1];
  txid[2] = txid_[2];
  sum = txid[0] + txid[1] + txid[2];
  // Base row is defined by lowest 2 bits
  uint8_t (&fh_row)[16] = freq_hopping[sum & 0x03];
  // Higher 3 bits define increment to corresponding row
  uint8_t increment = (sum & 0x1e) >> 2;
  for (int i = 0; i < 16; ++i) {
    uint8_t val = fh_row[i] + increment;
    // Strange avoidance of channels divisible by 16
    rf_channels[i] = (val & 0x0f) ? val : val - 3;
  }
}

void V202_TX::begin()
{
  radio.begin();
  radio.write_register(CONFIG, _BV(EN_CRC) | _BV(CRCO));
  radio.write_register(EN_AA, 0x00);
  radio.write_register(EN_RXADDR, 0x3F);
  radio.write_register(SETUP_AW, 0x03);
  radio.write_register(SETUP_RETR, 0xFF);
  radio.write_register(RF_CH, 0x08);
  radio.write_register(RF_SETUP, 0x05); // 0x05 - 1Mbps, 0dBm power, LNA high gaim
  radio.write_register(STATUS, 0x70);
  radio.write_register(OBSERVE_TX, 0x00);
  radio.write_register(CD, 0x00);
  radio.write_register(RX_ADDR_P2, 0xC3);
  radio.write_register(RX_ADDR_P3, 0xC4);
  radio.write_register(RX_ADDR_P4, 0xC5);
  radio.write_register(RX_ADDR_P5, 0xC6);
  radio.write_register(RX_PW_P0, 0x10);
  radio.write_register(RX_PW_P1, 0x10);
  radio.write_register(RX_PW_P2, 0x10);
  radio.write_register(RX_PW_P3, 0x10);
  radio.write_register(RX_PW_P4, 0x10);
  radio.write_register(RX_PW_P5, 0x10);
  radio.write_register(FIFO_STATUS, 0x00);
  const uint8_t* rx_tx_addr = reinterpret_cast<const uint8_t *>("\x66\x88\x68\x68\x68");
  const uint8_t* rx_p1_addr = reinterpret_cast<const uint8_t *>("\x88\x66\x86\x86\x86");
  radio.write_register(RX_ADDR_P0, rx_tx_addr, 5);
  radio.write_register(RX_ADDR_P1, rx_p1_addr, 5);
  radio.write_register(TX_ADDR, rx_tx_addr, 5);
  // Check for Beken BK2421/BK2423 chip
  // It is done by using Beken specific activate code, 0x53
  // and checking that status register changed appropriately
  // There is no harm to run it on nRF24L01 because following
  // closing activate command changes state back even if it
  // does something on nRF24L01
  radio.activate(0x53); // magic for BK2421 bank switch
  Serial.write("Try to switch banks "); Serial.print(radio.read_register(STATUS)); Serial.write("\n");
  if (radio.read_register(STATUS) & 0x80) {
    Serial.write("BK2421!\n");
    long nul = 0;
    radio.write_register(0x00, (const uint8_t *) "\x40\x4B\x01\xE2", 4);
    radio.write_register(0x01, (const uint8_t *) "\xC0\x4B\x00\x00", 4);
    radio.write_register(0x02, (const uint8_t *) "\xD0\xFC\x8C\x02", 4);
    radio.write_register(0x03, (const uint8_t *) "\xF9\x00\x39\x21", 4);
    radio.write_register(0x04, (const uint8_t *) "\xC1\x96\x9A\x1B", 4);
    radio.write_register(0x05, (const uint8_t *) "\x24\x06\x7F\xA6", 4);
    radio.write_register(0x06, (const uint8_t *) &nul, 4);
    radio.write_register(0x07, (const uint8_t *) &nul, 4);
    radio.write_register(0x08, (const uint8_t *) &nul, 4);
    radio.write_register(0x09, (const uint8_t *) &nul, 4);
    radio.write_register(0x0A, (const uint8_t *) &nul, 4);
    radio.write_register(0x0B, (const uint8_t *) &nul, 4);
    radio.write_register(0x0C, (const uint8_t *) "\x00\x12\x73\x00", 4);
    radio.write_register(0x0D, (const uint8_t *) "\x46\xB4\x80\x00", 4);
    radio.write_register(0x0E, (const uint8_t *) "\x41\x10\x04\x82\x20\x08\x08\xF2\x7D\xEF\xFF", 11);
    radio.write_register(0x04, (const uint8_t *) "\xC7\x96\x9A\x1B", 4);
    radio.write_register(0x04, (const uint8_t *) "\xC1\x96\x9A\x1B", 4);
  }
  radio.activate(0x53); // switch bank back
  
  delay(50);
  radio.flush_tx();
  radio.write_register(CONFIG, _BV(EN_CRC) | _BV(CRCO) | _BV(PWR_UP));
  delayMicroseconds(150);
//  packet_sent = true;
  rf_ch_num = 0;
  // This is a bogus packet which actual V202 TX sends, but it is
  // probably some garbage, so no need to send it.
//  uint8_t buf[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9B,
//                      0x9C, 0x88, 0x48, 0x8F, 0xD3, 0x00, 0xDA, 0x8F };
//  radio.flush_tx();
//  radio.write_payload(buf, 16);
  radio.ce(HIGH);
//  delayMicroseconds(15);
  // It saves power to turn off radio after the transmission,
  // so as long as we have pins to do so, it is wise to turn
  // it back.
//  radio.ce(LOW);
}

void V202_TX::command(uint8_t throttle, int8_t yaw, int8_t pitch, int8_t roll, uint8_t flags)
{
  uint8_t buf[16];
  if (flags == 0xc0) {
    // binding
    buf[0] = 0x00;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x00;
  } else {
    // regular packet
    buf[0] = throttle;
    buf[1] = (uint8_t) yaw;
    buf[2] = (uint8_t) pitch;
    buf[3] = (uint8_t) roll;
    // Trims, middle is 0x40
    buf[4] = 0x40; // yaw
    buf[5] = 0x40; // pitch
    buf[6] = 0x40; // roll
  }
  // TX id
  buf[7] = txid[0];
  buf[8] = txid[1];
  buf[9] = txid[2];
  // empty
  buf[10] = 0x00;
  buf[11] = 0x00;
  buf[12] = 0x00;
  buf[13] = 0x00;
  //
  buf[14] = flags;
  uint8_t sum = 0;
  uint8_t i;
  for (i = 0; i < 15;  ++i) sum += buf[i];
  buf[15] = sum;
  if (packet_sent) {
    bool report_done = false;
//    if  (!(radio.read_register(STATUS) & _BV(TX_DS))) { Serial.write("Waiting for radio\n"); report_done = true; }
    while (!(radio.read_register(STATUS) & _BV(TX_DS))) ;
    radio.write_register(STATUS, _BV(TX_DS));
//    if (report_done) Serial.write("Done\n");
  }
  packet_sent = true;
  // Each packet is repeated twice on the same
  // channel, hence >> 1
  // We're not strictly repeating them, rather we
  // send new packet on the same frequency, so the
  // receiver gets the freshest command. As receiver
  // hops to a new frequency as soon as valid packet
  // received it does not matter that the packet is
  // not the same one repeated twice - nobody checks this
  uint8_t rf_ch = rf_channels[rf_ch_num >> 1];
  rf_ch_num++; if (rf_ch_num >= 32) rf_ch_num = 0;
//  Serial.print(rf_ch); Serial.write("\n");
  radio.write_register(RF_CH, rf_ch);
  radio.flush_tx();
  radio.write_payload(buf, 16);
  //radio.ce(HIGH);
  delayMicroseconds(15);
  //radio.ce(LOW);
}

