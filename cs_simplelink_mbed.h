/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */
#ifndef CS_SL_MBED_H_
#define CS_SL_MBED_H_

#define SL_SPI_DEFAULT_FREQ 16000000

/*
 * we cannot turn on SL_INC_STD_BSD_API_NAMING because excessive use
 * of preprocessor clashes with C++, e.g. `connect` is also a C++ method name)
 */
#define socklen_t SlSocklen_t
/* defined by mbed system headers */
/* #define timeval                             SlTimeval_t */
#define sockaddr SlSockAddr_t
#define in6_addr SlIn6Addr_t
#define sockaddr_in6 SlSockAddrIn6_t
#define in_addr SlInAddr_t
#define sockaddr_in SlSockAddrIn_t

#define SOCK_STREAM SL_SOCK_STREAM
#define SOCK_DGRAM SL_SOCK_DGRAM

#define AF_INET SL_AF_INET
#define AF_INET6 SL_AF_INET6
#define INADDR_ANY SL_INADDR_ANY

#define htonl sl_Htonl
#define ntohl sl_Ntohl
#define htons sl_Htons
#define ntohs sl_Ntohs

#if defined(__cplusplus)

class SimpleLinkInterface : public WiFiInterface {
 public:
  SimpleLinkInterface(PinName nHIB, PinName irq, int freq = SL_SPI_DEFAULT_FREQ,
                      bool debug = false);

  SimpleLinkInterface(PinName nHIB, PinName irq, PinName mosi, PinName miso,
                      PinName sclk, PinName cs, int freq = SL_SPI_DEFAULT_FREQ,
                      bool debug = false);

  virtual int connect();
  virtual int connect(const char *ssid, const char *pass,
                      nsapi_security_t security = NSAPI_SECURITY_NONE,
                      uint8_t channel = 0);
  virtual int set_credentials(const char *ssid, const char *pass,
                              nsapi_security_t security = NSAPI_SECURITY_NONE);
  virtual int set_channel(uint8_t channel);
  virtual int disconnect();
  virtual const char *get_ip_address();
  virtual const char *get_mac_address();
  virtual const char *get_gateway();
  virtual int8_t get_rssi();
  virtual int scan(WiFiAccessPoint *res, unsigned count);

  virtual NetworkStack *get_stack();

 private:
  void init(PinName nHIB, PinName irq, PinName mosi, PinName miso, PinName sclk,
            PinName cs, int freq, bool debug);

  const char *ssid_;
  const char *pass_;
  nsapi_security_t security_;
  uint8_t channel_;
};

#endif

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

typedef int cs_sl_fd_t;

typedef void (*SL_P_EVENT_HANDLER)(void);

void mbed_sl_DeviceEnablePreamble();
void mbed_sl_DeviceEnable();
void mbed_sl_DeviceDisable();
cs_sl_fd_t mbed_sl_IfOpen(char *ifname, int flags);
int mbed_sl_IfClose(cs_sl_fd_t fd);
int mbed_sl_IfRead(cs_sl_fd_t Fd, unsigned char *pBuff, int Len);
int mbed_sl_IfWrite(cs_sl_fd_t Fd, unsigned char *pBuff, int Len);

void mbed_sl_IfMaskIntHdlr();
void mbed_sl_IfUnMaskIntHdlr();
int mbed_sl_IfRegIntHdlr(SL_P_EVENT_HANDLER InterruptHdl, void *pValue);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_SL_MBED_H_ */
