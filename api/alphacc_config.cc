#include <fstream>

#include "api/alphacc_config.h"
#include "rtc_base/strings/json.h"

#define RETURN_ON_FAIL(success) \
  do {                          \
    bool result = (success);    \
    if (!result) {              \
      return false;             \
    }                           \
  } while (0)

namespace webrtc {
// alphaCC global configurations
static AlphaCCConfig* config;

const AlphaCCConfig* GetAlphaCCConfig() {
  return config;
}

bool ParseAlphaCCConfig(const std::string& file_path) {
  if (!config) {
    config = new AlphaCCConfig();
  }

  Json::Reader reader;
  Json::Value top;
  Json::Value second;
  Json::Value third;
  std::ifstream is(file_path);

  auto GetString = ::rtc::GetStringFromJsonObject;
  auto GetBool = ::rtc::GetBoolFromJsonObject;
  auto GetInt = ::rtc::GetIntFromJsonObject;
  auto GetValue = ::rtc::GetValueFromJsonObject;

  RETURN_ON_FAIL(reader.parse(is, top));

  if (GetValue(top, "server_connection", &second)) {
    RETURN_ON_FAIL(GetString(second, "ip", &config->conn_server_ip));
    RETURN_ON_FAIL(GetInt(second, "port", &config->conn_server_port));
    RETURN_ON_FAIL(GetBool(second, "autoconnect", &config->conn_autoconnect));
    RETURN_ON_FAIL(GetBool(second, "autocall", &config->conn_autocall));
    RETURN_ON_FAIL(GetInt(second, "autoclose", &config->conn_autoclose));
  }
  second.clear();

  if (GetValue(top, "serverless_connection", &second)) {
    RETURN_ON_FAIL(GetInt(second, "autoclose", &config->conn_autoclose));
    RETURN_ON_FAIL(GetValue(second, "sender", &third));
    RETURN_ON_FAIL(GetBool(third, "enabled", &config->is_sender));
    if (config->is_sender) {
      RETURN_ON_FAIL(GetString(third, "dest_ip", &config->dest_ip));
      RETURN_ON_FAIL(GetInt(third, "dest_port", &config->dest_port));
    }
    third.clear();
    RETURN_ON_FAIL(GetValue(second, "receiver", &third));
    RETURN_ON_FAIL(GetBool(third, "enabled", &config->is_receiver));
    if (config->is_receiver) {
      RETURN_ON_FAIL(GetString(third, "listening_ip", &config->listening_ip));
      RETURN_ON_FAIL(GetInt(third, "listening_port", &config->listening_port));
    }
    third.clear();
  }
  second.clear();

  RETURN_ON_FAIL(
      GetInt(top, "bwe_feedback_duration", &config->bwe_feedback_duration_ms));

  if (GetValue(top, "onnx", &second)) {
    GetString(second, "onnx_model_path", &config->onnx_model_path);
    second.clear();
  }

  bool enabled = false;
  RETURN_ON_FAIL(GetValue(top, "video_source", &second));
  RETURN_ON_FAIL(GetValue(second, "video_disabled", &third));
  RETURN_ON_FAIL(GetBool(third, "enabled", &enabled));
  if (enabled) {
    config->video_source_option =
        AlphaCCConfig::VideoSourceOption::kVideoDisabled;
  } else {
    third.clear();
    RETURN_ON_FAIL(GetValue(second, "webcam", &third));
    RETURN_ON_FAIL(GetBool(third, "enabled", &enabled));
    if (enabled) {
      config->video_source_option = AlphaCCConfig::VideoSourceOption::kWebcam;
    } else {
      third.clear();
      RETURN_ON_FAIL(GetValue(second, "video_file", &third));
      RETURN_ON_FAIL(GetBool(third, "enabled", &enabled));
      if (!enabled) {
        return false;
      }
      config->video_source_option =
          AlphaCCConfig::VideoSourceOption::kVideoFile;
      RETURN_ON_FAIL(GetInt(third, "height", &config->video_height));
      RETURN_ON_FAIL(GetInt(third, "width", &config->video_width));
      RETURN_ON_FAIL(GetInt(third, "fps", &config->video_fps));
      RETURN_ON_FAIL(GetString(third, "file_path", &config->video_file_path));
    }
  }
  third.clear();
  second.clear();
  enabled = false;
  RETURN_ON_FAIL(GetValue(top, "audio_source", &second));
  RETURN_ON_FAIL(GetValue(second, "microphone", &third));
  RETURN_ON_FAIL(GetBool(third, "enabled", &enabled));
  if (enabled) {
    config->audio_source_option = AlphaCCConfig::AudioSourceOption::kMicrophone;
  } else {
    third.clear();
    RETURN_ON_FAIL(GetValue(second, "audio_file", &third));
    RETURN_ON_FAIL(GetBool(third, "enabled", &enabled));
    if (enabled) {
      config->audio_source_option =
          AlphaCCConfig::AudioSourceOption::kAudioFile;
      RETURN_ON_FAIL(GetString(third, "file_path", &config->audio_file_path));
    } else {
      return false;
    }
  }

  second.clear();
  third.clear();
  RETURN_ON_FAIL(GetValue(top, "save_to_file", &second));
  RETURN_ON_FAIL(GetBool(second, "enabled", &config->save_to_file));
  if (config->save_to_file) {
    RETURN_ON_FAIL(GetValue(second, "video", &third));
    RETURN_ON_FAIL(GetString(third, "file_path", &config->video_output_path));
    RETURN_ON_FAIL(GetInt(third, "height", &config->video_output_height));
    RETURN_ON_FAIL(GetInt(third, "width", &config->video_output_width));
    RETURN_ON_FAIL(GetInt(third, "fps", &config->video_output_fps));

    third.clear();
    RETURN_ON_FAIL(GetValue(second, "audio", &third));
    RETURN_ON_FAIL(GetString(third, "file_path", &config->audio_output_path));
  }

  second.clear();
  third.clear();
  RETURN_ON_FAIL(GetValue(top, "logging", &second));
  RETURN_ON_FAIL(GetBool(second, "enabled", &config->save_log_to_file));
  if (config->save_log_to_file) {
    RETURN_ON_FAIL(GetString(second, "log_output_path", &config->log_output_path));
  }

  return true;
}

}  // namespace webrtc
