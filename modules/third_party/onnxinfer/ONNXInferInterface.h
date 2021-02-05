#ifndef ONNX_INFER_ONNX_INFER_INTERFACE_H_
#define ONNX_INFER_ONNX_INFER_INTERFACE_H_

#ifdef _WIN32
#ifdef ONNXInferInterface_DLL_Provider_
#define ONNXInferInterface_DLL_EXPORT_IMPORT_ __declspec(dllexport)
#else
#define ONNXInferInterface_DLL_EXPORT_IMPORT_ __declspec(dllimport)
#endif
#else
#define ONNXInferInterface_DLL_EXPORT_IMPORT_
#endif

#ifdef __cplusplus
extern "C" {
#endif
    namespace onnxinfer {

        ONNXInferInterface_DLL_EXPORT_IMPORT_
            void OnReceived(
                void* onnx_infer_interface,
                unsigned char payloadType,
                unsigned short sequenceNumber,
                unsigned int sendTimestamp, // ms
                unsigned int ssrc,
                unsigned long paddingLength,
                unsigned long headerLength,
                unsigned long long arrivalTimestamp, // ms
                unsigned long payloadSize, // bytes
                int lossCount, // packet, -1 indicates no valid lossCount from RTC
                float rtt); // sec, -1 indicates no valid rtt from RTC

        ONNXInferInterface_DLL_EXPORT_IMPORT_
            float GetBweEstimate(void* onnx_infer_interface); // bps

        ONNXInferInterface_DLL_EXPORT_IMPORT_
            const char* GetInfo(void* onnx_infer_interface);

        ONNXInferInterface_DLL_EXPORT_IMPORT_
            bool IsReady(void* onnx_infer_interface);

        ONNXInferInterface_DLL_EXPORT_IMPORT_
            void* CreateONNXInferInterface(const char* model_path);

        ONNXInferInterface_DLL_EXPORT_IMPORT_
            void DestroyONNXInferInterface(void* onnx_infer_interface);

    } //namespace onnxinfer
#ifdef __cplusplus
} //extern "C"
#endif
#endif // ONNX_INFER_ONNX_INFER_INTERFACE_H_