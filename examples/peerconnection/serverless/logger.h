#include "rtc_base/logging.h"

class FileLogSink : public rtc::LogSink {
  void OnLogMessage(const std::string& message);
  void OnLogMessage(const std::string& msg,
                    rtc::LoggingSeverity severity,
                    const char* tag);
  void OnLogMessage(const std::string& msg,
                    rtc::LoggingSeverity /* severity */);

 private:
  std::string LOG_FILEPATH = "webrtc.log";
};