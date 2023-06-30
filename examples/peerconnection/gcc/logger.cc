#include "logger.h"
#include <iostream>

FileLogSink::FileLogSink(const std::string& log_filepath)
    : log_filepath_(log_filepath) {
  log_file_ = fopen(log_filepath_.c_str(), "a");
  if (log_file_ != NULL) {
    rtc::LogMessage::AddLogToStream(this, rtc::LoggingSeverity::INFO);
  }
}

FileLogSink::~FileLogSink() {
  if (log_file_ != NULL) {
    fclose(log_file_);
    rtc::LogMessage::RemoveLogToStream(this);
  }
}

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
  fwrite(message.c_str(), message.length(), 1, log_file_);
  fflush(log_file_);
}