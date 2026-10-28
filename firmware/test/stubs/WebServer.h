#pragma once
#include <functional>
#include "Arduino.h"
enum HTTPMethod { HTTP_ANY, HTTP_GET, HTTP_POST };
enum HTTPAuthMethod { BASIC_AUTH, DIGEST_AUTH };
enum HTTPUploadStatus { UPLOAD_FILE_START, UPLOAD_FILE_WRITE, UPLOAD_FILE_END, UPLOAD_FILE_ABORTED };
#define CONTENT_LENGTH_UNKNOWN ((size_t)-1)
struct HTTPUpload {
  HTTPUploadStatus status;
  String filename;
  size_t totalSize;
  size_t currentSize;
  uint8_t buf[1436];
};
class WebServer {
 public:
  typedef std::function<void(void)> THandlerFunction;
  explicit WebServer(int port = 80);
  void on(const char* uri, HTTPMethod method, THandlerFunction fn);
  void on(const char* uri, HTTPMethod method, THandlerFunction fn, THandlerFunction ufn);
  void onNotFound(THandlerFunction fn);
  void begin();
  void handleClient();
  bool hasArg(const String& name) const;
  String arg(const String& name) const;
  bool authenticate(const char* username, const char* password);
  void requestAuthentication(HTTPAuthMethod mode = BASIC_AUTH, const char* realm = nullptr,
                             const String& authFailMsg = String(""));
  void sendHeader(const String& name, const String& value, bool first = false);
  void send(int code, const char* content_type = nullptr, const String& content = String(""));
  void send_P(int code, PGM_P content_type, PGM_P content);
  void setContentLength(const size_t contentLength);
  void sendContent(const String& content);
  HTTPUpload& upload();
};
