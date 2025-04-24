#include "inference.h"
#include "inference_openvino.h"
#include "inference_dlib.h"
InferenceOV* openvino_inference;
InferenceDLIB* dlib_inference;
Inference* Inference::get_inference(InferenceType inf_type)
{
	if (inf_type == INF_OPENVINO) {
		if (!openvino_inference) openvino_inference = new InferenceOV;
		return openvino_inference;
	}
	if (inf_type == INF_DLIB) {
		if (!dlib_inference) dlib_inference = new InferenceDLIB;
		return dlib_inference;
	}
	return nullptr;
}
