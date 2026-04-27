#include "NTRIPClient.h"

extern int LOG_LEVEL;

#define NTRIP_LOG(level, msg)   do { if (LOG_LEVEL >= (level)) Serial.print(msg); } while (0)
#define NTRIP_LOGLN(level, msg) do { if (LOG_LEVEL >= (level)) Serial.println(msg); } while (0)

NTRIPClient::NTRIPClient(Client& transport)
  : client(transport) {}

int NTRIPClient::connect(IPAddress ip, uint16_t port) {
  return client.connect(ip, port);
}

int NTRIPClient::connect(const char* host, uint16_t port) {
  return client.connect(host, port);
}

size_t NTRIPClient::write(uint8_t value) {
  return client.write(value);
}

size_t NTRIPClient::write(const uint8_t* buf, size_t size) {
  return client.write(buf, size);
}

int NTRIPClient::available() {
  return client.available();
}

int NTRIPClient::read() {
  return client.read();
}

int NTRIPClient::read(uint8_t* buf, size_t size) {
  return client.read(buf, size);
}

int NTRIPClient::peek() {
  return client.peek();
}

void NTRIPClient::flush() {
  client.flush();
}

void NTRIPClient::stop() {
  client.stop();
}

uint8_t NTRIPClient::connected() {
  return client.connected();
}

NTRIPClient::operator bool() {
  return static_cast<bool>(client);
}

bool NTRIPClient::waitForData(uint32_t timeoutMs) {
  const uint32_t start = millis();
  while (available() == 0) {
    if ((millis() - start) > timeoutMs) {
      return false;
    }
    delay(10);
  }
  return true;
}

bool NTRIPClient::reqSrcTbl(const char* host, uint16_t port) {
  stop();

  if (!connect(host, port)) {
    NTRIP_LOG(1, "Cannot connect to ");
    NTRIP_LOGLN(1, host);
    return false;
  }

  String req;
  req.reserve(192);
  req += F("GET / HTTP/1.0\r\n");
  req += F("Host: ");
  req += host;
  req += ':';
  req += String(port);
  req += F("\r\n");
  req += F("User-Agent: NTRIPClient for ESP32/TinyGSM\r\n");
  req += F("Accept: */*\r\n");
  req += F("Connection: close\r\n\r\n");

  print(req);

  if (!waitForData(5000)) {
    NTRIP_LOGLN(1, "Client Timeout while requesting source table");
    stop();
    return false;
  }

  char buffer[96];
  readLine(buffer, sizeof(buffer), 1000);

  if (strstr(buffer, "SOURCETABLE 200 OK") != nullptr) {
    return true;
  }

  NTRIP_LOG(1, "Unexpected sourcetable response: ");
  NTRIP_LOGLN(1, buffer);
  stop();
  return false;
}

bool NTRIPClient::reqRaw(const char* host, uint16_t port, const char* mntpnt) {
  return reqRaw(host, port, mntpnt, "", "");
}

bool NTRIPClient::reqRaw(const char* host, uint16_t port, const char* mntpnt, const char* user, const char* psw) {
  stop();

  if (!connect(host, port)) {
    NTRIP_LOG(1, "Cannot connect to ");
    NTRIP_LOG(1, host);
    NTRIP_LOG(1, ':');
    NTRIP_LOGLN(1, port);
    return false;
  }

  NTRIP_LOGLN(1, "Request NTRIP RAW stream");

  String req;
  req.reserve(256);
  req += F("GET /");
  req += mntpnt;
  req += F(" HTTP/1.0\r\n");
  req += F("Host: ");
  req += host;
  req += ':';
  req += String(port);
  req += F("\r\n");
  req += F("User-Agent: NTRIPClient for ESP32/TinyGSM\r\n");
  req += F("Accept: */*\r\n");
  req += F("Connection: close\r\n");

  if (user != nullptr && user[0] != '\0') {
    const String auth = base64::encode(String(user) + ':' + String(psw ? psw : ""));
    req += F("Authorization: Basic ");
    req += auth;
    req += F("\r\n");
  }

  req += F("\r\n");

  print(req);
  flush();

  if (!waitForData(20000)) {
    NTRIP_LOGLN(1, "Client Timeout waiting for NTRIP reply");
    stop();
    return false;
  }

  char buffer[96];
  readLine(buffer, sizeof(buffer), 2000);

  if (strncmp(buffer, "ICY 200 OK", 10) == 0 || strstr(buffer, "HTTP/1.1 200") != nullptr || strstr(buffer, "HTTP/1.0 200") != nullptr) {
    return true;
  }

  NTRIP_LOG(1, "Unexpected NTRIP response: ");
  NTRIP_LOGLN(1, buffer);
  stop();
  return false;
}

int NTRIPClient::readLine(char* buffer, int size, uint32_t timeoutMs) {
  if (buffer == nullptr || size <= 0) {
    return 0;
  }

  const uint32_t start = millis();
  int len = 0;

  while (len < (size - 1)) {
    while (available() == 0) {
      if ((millis() - start) > timeoutMs) {
        buffer[len] = '\0';
        return len;
      }
      delay(1);
    }

    const int c = read();
    if (c < 0) {
      break;
    }

    buffer[len++] = static_cast<char>(c);
    if (c == '\n') {
      break;
    }
  }

  buffer[len] = '\0';
  return len;
}

bool NTRIPClient::sendGGA(const char* ggaMessage) {
  if (ggaMessage == nullptr || ggaMessage[0] == '\0') {
    return false;
  }

  if (!connected()) {
    NTRIP_LOGLN(1, "NTRIPClient not connected, cannot send GGA");
    return false;
  }

  size_t written = print(ggaMessage);
  if (written == 0) {
    return false;
  }

  const size_t len = strlen(ggaMessage);
  if (len < 2 || strcmp(ggaMessage + len - 2, "\r\n") != 0) {
    print("\r\n");
  }

  flush();
  return true;
}

bool NTRIPClient::sendGGA(const char* ggaMessage, const char* host, uint16_t port, const char* user, const char* passwd, const char* mntpnt) {
  if (!connected()) {
    if (!reqRaw(host, port, mntpnt, user, passwd)) {
      return false;
    }
  }
  return sendGGA(ggaMessage);
}

void NTRIPClient::enqueueGGA(const String& message) {
  lastGGA = message;
}

void NTRIPClient::setLastGGA(const String& gga) {
  lastGGA = gga;
}

String NTRIPClient::getLastGGA() const {
  return lastGGA;
}