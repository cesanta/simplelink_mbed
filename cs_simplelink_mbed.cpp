/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "mbed.h"
#include <assert.h>
#include "simplelink/include/simplelink.h"

// see mbed's SPI::format doc
#define SL_SPI_CLOCK_ACTIVE_HIGH_RISING 0

#define SL_SPI_WORD_SIZE 8
#define SL_SPI_CLOCK_POLARITY_PHASE SL_SPI_CLOCK_ACTIVE_HIGH_RISING

class CC3100;
static CC3100 *cc;

class CC3100 {
  friend class SimpleLinkInterface;

 public:
  CC3100(PinName nHIB, PinName irq, PinName mosi, PinName miso, PinName sclk,
         PinName cs, int freq, bool debug);

  void start();
  int wlan_connect(const char *ssid, const char *pass);

  void handle_irq();

  void device_enable();
  void device_disable();
  void register_interrupt_handler(SL_P_EVENT_HANDLER InterruptHdlr,
                                  void *pValue);
  void mask_irq();
  void unmask_irq();

  int read(char *buf, int len);
  int write(char *buf, int len);

  void wlan_event_handler(SlWlanEvent_t *pWlanEvent);
  void netapp_event_handler(SlNetAppEvent_t *pNetAppEvent);

 protected:
  void debugf(const char *fmt, ...);

 private:
  bool connected_;
  uint8_t mac_[6];
  uint32_t ip_;
  uint32_t gw_;
  uint32_t dns_;
  uint8_t rssi_;

  bool debug_;

  SPI spi_;
  DigitalOut cs_;
  DigitalOut nHIB_;
  InterruptIn irq_;

  SL_P_EVENT_HANDLER irq_handler_;
};

CC3100::CC3100(PinName nHIB, PinName irq, PinName mosi, PinName miso,
               PinName sclk, PinName cs, int freq, bool debug)
    : connected_(false),
      ip_(0),
      gw_(0),
      rssi_(0),
      debug_(debug),
      spi_(mosi, miso, sclk),
      cs_(cs),
      nHIB_(nHIB),
      irq_(irq) {
  spi_.format(SL_SPI_WORD_SIZE, SL_SPI_CLOCK_POLARITY_PHASE);
  spi_.frequency(freq);

  irq_.mode(PullDown);  // irq goes Z when cc3200 is disabled
  irq_.rise(callback(this, &CC3100::handle_irq));
  nHIB_ = 0;
  cs_ = 1;

  // http://processors.wiki.ti.com/index.php/CC31xx_Host_Interface
  // claims 10 ms as minimum hibernation time, let's double it,
  // what's 10 ms between friends.
  wait_ms(20);
}

void CC3100::debugf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (debug_) {
    vprintf(fmt, ap);
  }
  va_end(ap);
}

void CC3100::start() {
  int ret = sl_Start(NULL, NULL, NULL);
  debugf("SL: sl_Start -> %d\n", ret);
}

void CC3100::handle_irq() {
  irq_handler_();
}

void CC3100::device_enable() {
  debugf("SL: device enable\n");
  nHIB_ = 1;
}

void CC3100::device_disable() {
  debugf("SL: device disable\n");
  nHIB_ = 0;
}

void CC3100::register_interrupt_handler(SL_P_EVENT_HANDLER InterruptHdlr,
                                        void *pValue) {
  irq_handler_ = InterruptHdlr;
}

void CC3100::mask_irq() {
}

void CC3100::unmask_irq() {
}

int CC3100::read(char *buf, int len) {
  debugf("SL: read %d bytes from SPI\n", len);
  cs_ = 0;
  for (int i = 0; i < len; i++) {
    buf[i] = spi_.write(0xFF);
  }
  cs_ = 1;
  return len;
}

int CC3100::write(char *buf, int len) {
  debugf("SL: write %d bytes to SPI\n", len);
  cs_ = 0;
  for (int i = 0; i < len; i++) {
    spi_.write(buf[i]);
  }
  cs_ = 1;
  return len;
}

int CC3100::wlan_connect(const char *ssid, const char *pass) {
  SlSecParams_t sec_params;
  memset(&sec_params, 0, sizeof(sec_params));
  sec_params.Key = (signed char *) pass;
  sec_params.KeyLen = strlen(pass);
  sec_params.Type = SL_SEC_TYPE_WPA_WPA2;

  uint8_t macLen = sizeof(mac_);
  sl_NetCfgGet(SL_MAC_ADDRESS_GET, NULL, &macLen, mac_);

  debugf("SL: starting WiFi connect\n");
  return sl_WlanConnect((signed char *) ssid, strlen(ssid), 0, &sec_params,
                        NULL);
}

void CC3100::wlan_event_handler(SlWlanEvent_t *pWlanEvent) {
  switch (pWlanEvent->Event) {
    case SL_WLAN_CONNECT_EVENT:
      debugf("SL: connected to WiFi\n");
      connected_ = true;
      break;
    case SL_WLAN_DISCONNECT_EVENT:
      debugf("SL: disconnected WiFi\n");
      connected_ = false;
      break;
    default:
      debugf("SL: got wlan event %lu\n", pWlanEvent->Event);
      break;
  }
}

void CC3100::netapp_event_handler(SlNetAppEvent_t *pNetAppEvent) {
  switch (pNetAppEvent->Event) {
    case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
      debugf("SL: got IP\n");
      ip_ = pNetAppEvent->EventData.ipAcquiredV4.ip;
      gw_ = pNetAppEvent->EventData.ipAcquiredV4.gateway;
      dns_ = pNetAppEvent->EventData.ipAcquiredV4.dns;
      break;
    default:
      debugf("SL: got NetApp event: %lu\n", pNetAppEvent->Event);
      break;
  }
}

/* SimpleLink mbed wifi interface implementation */

SimpleLinkInterface::SimpleLinkInterface(PinName nHIB, PinName irq, int freq,
                                         bool debug) {
  init(nHIB, irq, SPI_MOSI, SPI_MISO, SPI_SCK, SPI_CS, freq, debug);
}

SimpleLinkInterface::SimpleLinkInterface(PinName nHIB, PinName irq,
                                         PinName mosi, PinName miso,
                                         PinName sclk, PinName cs, int freq,
                                         bool debug) {
  init(nHIB, irq, mosi, miso, sclk, cs, freq, debug);
}

/* the mbed online compiler doesn't support c++11 */
void SimpleLinkInterface::init(PinName nHIB, PinName irq, PinName mosi,
                               PinName miso, PinName sclk, PinName cs, int freq,
                               bool debug) {
  assert(cc == NULL);
  cc = new CC3100(nHIB, irq, mosi, miso, sclk, cs, freq, debug);
  cc->start();
}

int SimpleLinkInterface::set_credentials(const char *ssid, const char *pass,
                                         nsapi_security_t security) {
  ssid_ = ssid;
  pass_ = pass;
  security_ = security;
  return 0;
}

int SimpleLinkInterface::set_channel(uint8_t channel) {
  channel_ = channel;
  return 0;
}

int SimpleLinkInterface::connect(const char *ssid, const char *pass,
                                 nsapi_security_t security, uint8_t channel) {
  set_credentials(ssid, pass, security);
  set_channel(channel);
  int res = connect();
  if (res != 0) {
    return res;
  }

  while (true) {
    if (cc->ip_ != 0) {
      break;
    }
    _SlNonOsMainLoopTask();
    wait(0.1);
  }
  return 0;
}

int SimpleLinkInterface::connect() {
  cc->wlan_connect(ssid_, pass_);
  return 0;
}

int SimpleLinkInterface::disconnect() {
  // TODO
  return 0;
}

static char *ip_to_str(char *buf, size_t len, uint32_t ip) {
  if (ip == 0) {
    return NULL;
  }
  snprintf(buf, len, "%lu.%lu.%lu.%lu", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
           (ip >> 8) & 0xFF, (ip >> 0) & 0xFF);
  return buf;
}

const char *SimpleLinkInterface::get_ip_address() {
  static char buf[4 * 4];
  return ip_to_str(buf, sizeof(buf), cc->ip_);
}

const char *SimpleLinkInterface::get_gateway() {
  static char buf[4 * 4];
  return ip_to_str(buf, sizeof(buf), cc->gw_);
}

const char *SimpleLinkInterface::get_mac_address() {
  static char buf[3 * 6];
  uint8_t *m = cc->mac_;
  snprintf(buf, sizeof(buf), "%X:%X:%X:%X:%X:%X", m[0], m[1], m[2], m[3], m[4],
           m[5]);
  return buf;
}

int8_t SimpleLinkInterface::get_rssi() {
  return cc->rssi_;
}

int SimpleLinkInterface::scan(WiFiAccessPoint *res, unsigned count) {
  return -1;
}

NetworkStack *SimpleLinkInterface::get_stack() {
  // don't implement a full mbed networking stack
  return NULL;
}

/* implement simplelink C hal */

void mbed_sl_DeviceEnablePreamble() {
}

void mbed_sl_DeviceEnable() {
  assert(cc != NULL);
  cc->device_enable();
}

void mbed_sl_DeviceDisable() {
  assert(cc != NULL);
  cc->device_disable();
}

cs_sl_fd_t mbed_sl_IfOpen(char *ifname, int flags) {
  // dummy fd, we need a global cc instance anyway becasue the
  // interrupt and enable/disable HAL functions are nullary.
  return 1;
}

int mbed_sl_IfClose(cs_sl_fd_t fd) {
  return 0;
}

int mbed_sl_IfRead(cs_sl_fd_t Fd, unsigned char *pBuff, int Len) {
  assert(cc != NULL);
  return cc->read((char *) pBuff, Len);
}

int mbed_sl_IfWrite(cs_sl_fd_t Fd, unsigned char *pBuff, int Len) {
  assert(cc != NULL);
  return cc->write((char *) pBuff, Len);
}

void mbed_sl_IfMaskIntHdlr() {
  assert(cc != NULL);
  cc->mask_irq();
}

void mbed_sl_IfUnMaskIntHdlr() {
  assert(cc != NULL);
  cc->unmask_irq();
}

int mbed_sl_IfRegIntHdlr(SL_P_EVENT_HANDLER InterruptHdlr, void *pValue) {
  assert(cc != NULL);
  cc->register_interrupt_handler(InterruptHdlr, pValue);
  return 0;
}

void mbed_sl_GeneralEvtHdlr(SlDeviceEvent_t *pSlDeviceEvent) {
}

void mbed_sl_WlanEvtHdlr(SlWlanEvent_t *pWlanEvent) {
  assert(cc != NULL);
  cc->wlan_event_handler(pWlanEvent);
}

void mbed_sl_NetAppEvtHdlr(SlNetAppEvent_t *pNetAppEvent) {
  assert(cc != NULL);
  cc->netapp_event_handler(pNetAppEvent);
}

void mbed_sl_HttpServerCallback(SlHttpServerEvent_t *pSlHttpServerEvent,
                                SlHttpServerResponse_t *pSlHttpServerResponse) {
}

void mbed_sl_SockEvtHdlr(SlSockEvent_t *pSlSockEvent) {
}
