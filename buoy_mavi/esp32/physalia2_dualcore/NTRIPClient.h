#ifndef NTRIP_CLIENT
#define NTRIP_CLIENT

#include <Arduino.h>
#include <Client.h>
#include <IPAddress.h>
#include <base64.h>

class NTRIPClient : public Client {
public:
  using Print::write;

  explicit NTRIPClient(Client& transport);

  // Client interface delegation
  int connect(IPAddress ip, uint16_t port) override;
  int connect(const char* host, uint16_t port) override;
  size_t write(uint8_t value) override;
  size_t write(const uint8_t* buf, size_t size) override;
  int available() override;
  int read() override;
  int read(uint8_t* buf, size_t size) override;
  int peek() override;
  void flush() override;
  void stop() override;
  uint8_t connected() override;
  operator bool();

  // NTRIP helpers
  bool reqSrcTbl(const char* host, uint16_t port);
  bool reqRaw(const char* host, uint16_t port, const char* mntpnt, const char* user, const char* psw);
  bool reqRaw(const char* host, uint16_t port, const char* mntpnt);

  int readLine(char* buffer, int size, uint32_t timeoutMs = 1000);

  bool sendGGA(const char* ggaMessage);
  bool sendGGA(const char* ggaMessage, const char* host, uint16_t port, const char* user, const char* passwd, const char* mntpnt);

  void enqueueGGA(const String& message);
  void setLastGGA(const String& gga);
  String getLastGGA() const;

private:
  Client& client;
  String lastGGA;

  bool waitForData(uint32_t timeoutMs);
};

#endif
