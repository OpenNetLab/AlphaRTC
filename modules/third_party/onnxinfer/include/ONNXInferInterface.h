#ifndef ONNX_INFER_ONNX_INFER_INTERFACE_H_
#define ONNX_INFER_ONNX_INFER_INTERFACE_H_


#ifdef ONNXInferInterface_DLL_Provider_
#define ONNXInferInterface_DLL_EXPORT_IMPORT_ __declspec(dllexport)
#else
#define ONNXInferInterface_DLL_EXPORT_IMPORT_ __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif
    namespace onnxinfer {

        ONNXInferInterface_DLL_EXPORT_IMPORT_
            void OnReceived(
                void*              onnx_infer_interface,
                unsigned char      payloadType,
                unsigned short     sequenceNumber,
                unsigned int       sendTimestamp,
                unsigned int       ssrc,
                unsigned long      paddingLength,
                unsigned long      headerLength,
                unsigned long long arrivalTimeMs,
                unsigned long      payloadSize,
                float              lossRate);
        ONNXInferInterface_DLL_EXPORT_IMPORT_
            float GetBweEstimate(void* onnx_infer_interface);

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