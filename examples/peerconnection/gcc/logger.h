#include "rtc_base/logging.h"

class FileLogSink : public rtc::LogSink {
 public:
  FileLogSink(const std::string& log_filepath);
  ~FileLogSink();
  void OnLogMessage(const std::string& message);
  void OnLogMessage(const std::string& msg,
                    rtc::LoggingSeverity severity,
                    const char* tag);
  void OnLogMessage(const std::string& msg,
                    rtc::LoggingSeverity /* severity */);

 private:
  std::string log_filepath_;
  FILE* log_file_;
};