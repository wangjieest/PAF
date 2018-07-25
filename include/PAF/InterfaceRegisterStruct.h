#pragma once
#include "PAFConfig.h"
namespace PAF_NAMESPACE
{
using fn_create_t = void* (*)(void*);
using fn_destroy_t = void (*)(void*);
struct alignas(8) object_register_info_t
{
	fn_create_t create_obj_;
	fn_destroy_t destroy_obj_;
	PAF_OBJ_ID obj_id_;
	uint32_t flag_;
	uint32_t count_;
	const char* name_;
};
} // namespace PAF_NAMESPACE
