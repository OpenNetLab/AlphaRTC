#include "ONNXInferInterface.h"

namespace onnxinfer {
    void OnReceived(void* onnx_infer_interface,
                unsigned char payloadType,
                unsigned short sequenceNumber,
                unsigned int sendTimestamp, // ms
                unsigned int ssrc,
                unsigned long paddingLength,
                unsigned long headerLength,
                unsigned long long arrivalTimestamp, // ms
                unsigned long payloadSize, // bytes
                int lossCount, // packet, -1 indicates no valid lossCount from RTC
                float rtt) {
                    return;
                }
    float GetBweEstimate(void* onnx_infer_interface) {
        return 10000000;
    }

    const char* GetInfo(void* onnx_infer_interface) {
        return "none";
    }

    bool IsReady(void* onnx_infer_interface) {
        return true;
    }

    void* CreateONNXInferInterface(const char* model_path) {
        return nullptr;
    }

    void DestroyONNXInferInterface(void* onnx_infer_interface) {
        return;
    }
}