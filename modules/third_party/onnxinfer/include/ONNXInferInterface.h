#ifndef ONNX_INFER_ONNX_INFER_INTERFACE_H_
#define ONNX_INFER_ONNX_INFER_INTERFACE_H_


#ifdef DLL_Provider_
#define DLL_EXPORT_IMPORT_ __declspec(dllexport)
#else
#define DLL_EXPORT_IMPORT_ __declspec(dllimport)
#endif

namespace onnxinfer {

class DLL_EXPORT_IMPORT_ ONNXInferInterface {
public:
	virtual void OnReceived(
		//Packet Info
		unsigned char      payloadType,
		unsigned short     sequenceNumber,
		unsigned int       sendTimestamp,
		unsigned int       ssrc,
		unsigned long      paddingLength,
		unsigned long      headerLength,
		unsigned long long arrivalTimeMs,
		unsigned long      payloadSize,
		float              lossRate) = 0;

	virtual float GetBweEstimate() = 0;  // bps
	virtual ~ONNXInferInterface() {}

};

DLL_EXPORT_IMPORT_
ONNXInferInterface* CreateONNXInferInterface(const char* model_path);

DLL_EXPORT_IMPORT_
void DestroyONNXInferInterface(ONNXInferInterface* onnx_infer_interface);

} //namespace onnxinfer

#endif // ONNX_INFER_ONNX_INFER_INTERFACE_H_