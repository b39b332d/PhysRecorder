#include "inference.h"
#include "inference_openvino.h"
Inference* Inference::get_inference(InferenceType inf_type)
{
	return new InferenceOV();
}
