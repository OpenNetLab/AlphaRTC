#include <fstream>

#include "examples/peerconnection/client/alphacc_config.h"
#include "rtc_base/strings/json.h"

#define RETURN_ON_FAIL(success) \
  do {                          \
    bool result = (success);    \
    if (!result) {              \
      return false;             \
    }                           \
  } while (0)

namespace alphaCC {
// alphaCC global configurations
static AlphaCCConfig config;

const AlphaCCConfig* GetAlphaCCConfig() {
  return &config;
}

bool ParseAlphaCCConfig(std::string file_path) {
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

  RETURN_ON_FAIL(GetValue(top, "server_connection", &second));
  RETURN_ON_FAIL(GetString(second, "ip", &config.conn_server_ip));
  RETURN_ON_FAIL(GetInt(second, "port", &config.conn_server_port));
  RETURN_ON_FAIL(GetBool(second, "autoconnect", &config.conn_autoconnect));
  RETURN_ON_FAIL(GetBool(second, "autocall", &config.conn_autocall));
  RETURN_ON_FAIL(GetInt(second, "autoclose", &config.conn_autoclose));
  second.clear();

  RETURN_ON_FAIL(
      GetInt(top, "bwe_feedback_duration", &config.bwe_feedback_duration_ms));

  RETURN_ON_FAIL(GetValue(top, "onnx", &second));
  RETURN_ON_FAIL(GetString(second, "onnx_model_path", &config.onnx_model_path));
  second.clear();

  RETURN_ON_FAIL(GetValue(top, "redis", &second));
  RETURN_ON_FAIL(GetString(second, "ip", &config.redis_ip));
  RETURN_ON_FAIL(GetInt(second, "port", &config.redis_port));
  RETURN_ON_FAIL(GetString(second, "session_id", &config.redis_sid));
  RETURN_ON_FAIL(GetInt(second, "redis_update_duration",
                        &config.redis_update_duration_ms));
  second.clear();

  bool enabled = false;
  RETURN_ON_FAIL(GetValue(top, "video_source", &second));
  RETURN_ON_FAIL(GetValue(second, "video_disabled", &third));
  RETURN_ON_FAIL(GetBool(third, "enabled", &enabled));
  if (enabled) {
    config.video_source_option =
        AlphaCCConfig::VideoSourceOption::kVideoDisabled;
  } else {
    third.clear();
    RETURN_ON_FAIL(GetValue(second, "webcam", &third));
    RETURN_ON_FAIL(GetBool(third, "enabled", &enabled));
    if (enabled) {
      config.video_source_option = AlphaCCConfig::VideoSourceOption::kWebcam;
    } else {
      return false;
    }
  }
  return true;
}

}  // namespace alphaCC
