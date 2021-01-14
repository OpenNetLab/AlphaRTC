#include <iostream>
#include "logger.h"

void FileLogSink::OnLogMessage(const std::string& msg,
                           rtc::LoggingSeverity severity,
                           const char* tag) {
  OnLogMessage(tag + (": " + msg), severity);
}

void FileLogSink::OnLogMessage(const std::string& msg,
                           rtc::LoggingSeverity /* severity */) {
  OnLogMessage(msg);
}

void FileLogSink::OnLogMessage(const std::string& message) {
  FILE* log_file_ = fopen(LOG_FILEPATH.c_str(), "a");
  if (NULL == log_file_) {
    return;
  }
  fwrite(message.c_str(), message.length(), 1, log_file_);
  fflush(log_file_);
  fclose(log_file_);
}