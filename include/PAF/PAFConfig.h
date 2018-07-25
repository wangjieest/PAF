#pragma once
#include "CompileTimeHash.h"
#define PAF_COMPILE_TIME_HASH(name) COMPILE_TIME_HASH(name)

#define PAF_OBJ_ID uint64_t
#if !defined(PAF_LINK_API)
#	define PAF_LINK_API 
#endif  // !defined(PAF_LINK_API)

#define PAF_NAMESPACE paf_namespace
#define PAF_GET_GLOBAL_FACTORY paf_get_global_factory
#define PAF_DESTROY_GLOBAL_FACTORY paf_destroy_global_factory
#define PAF_ID_2_TYPE paf_id_2_type
#define PAF_TYPE_2_ID paf_type_2_id
#define PAF_GET_FACTORY PAF_NAMESPACE::PAF_GET_GLOBAL_FACTORY
#include <memory>
#define PAF_SHARED_PTR std::shared_ptr

#if !defined(PAF_ASSERT)
#	define PAF_ASSERT assert
#endif // !defined(PAF_ASSERT)

