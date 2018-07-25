#pragma once

#include "PAF/InterfaceRegisterStruct.h"
#include "PAF/TypeTraits.h"

namespace PAF_NAMESPACE
{
namespace detail
{
extern "C" PAF_LINK_API void reg_object_info(PAF_NAMESPACE::object_register_info_t& info);
} // namespace detail


template<typename T>
struct TypeToIndex
{
	enum
	{
		value =0
	};
};

template <typename T, typename INC, typename P, int Index = TypeToIndex<T>::value>
struct object_register_t;
#pragma pack(push, 4)
template <typename T, typename INC>
struct object_register_t<T, INC, PAF_NAMESPACE::detail::place_holder_t, 0>
{
	object_register_t(PAF_OBJ_ID id, uint8_t flag, const char* name)
	{
		fn_create_t _create_fun = [](void*) -> void* { return static_cast<INC*>(new T()); };
		fn_destroy_t _destroy_fun = [](void* data) -> void { delete (static_cast<T*>(data)); };
		object_register_info_t a{_create_fun, _destroy_fun, id, flag, 0, name};
		using detail::reg_object_info;
		reg_object_info(a);
	}
};
template <typename T, typename INC, typename... Args>
struct object_register_t<T, INC, PAF_NAMESPACE::detail::typelist_t<Args...>, 0>
{
	object_register_t(PAF_OBJ_ID id, uint8_t flag, const char* name)
	{
		fn_create_t _create_fun = [](void* t) -> void* {
			auto f = [](Args&... args) {
				return static_cast<INC*>(new T(detail::forward_t<Args>(args)...));
			};
			auto tup = static_cast<PAF_NAMESPACE::detail::typelist_t<Args...>*>(t);
			return detail::apply(f, *tup);
		};
		fn_destroy_t _destroy_fun = [](void* data) -> void { delete (static_cast<T*>(data)); };
		object_register_info_t a{_create_fun, _destroy_fun, id, flag, 0, name};
		using detail::reg_object_info;
		reg_object_info(a);
	}
};

#pragma pack(pop)
} // namespace PAF_NAMESPACE

#include "InterfaceDeclare.h"
#define REG_FACTORY_OBJECT_NAME_(name) PAF_REG_TYPE_##name
#define REG_FACTORY_OBJECT_NAME(name) REG_FACTORY_OBJECT_NAME_(name)

// auto register
#define PAF_REG_FACTORY_OBJECT_(INC, TYPE)                                                       \
	static_assert(PAF_TYPE_2_ID<INC>::id == PAF_COMPILE_TIME_HASH(INC), "id mismatch!");         \
	static_assert(std::is_base_of<PAF_TYPE_2_ID<INC>::type, TYPE>::value,                        \
				  "type '" PAF_TO_STR(TYPE) "' mismatch with interface '" PAF_TO_STR(INC) "'!"); \
	static_assert(std::is_same<INC, TYPE>::value || !std::has_virtual_destructor<INC>::value,    \
				  "interface : '" PAF_TO_STR(INC) "' should not has virtual dtor!!");            \
	extern "C" auto REG_FACTORY_OBJECT_NAME(INC) = PAF_NAMESPACE::                               \
		object_register_t<TYPE, INC, PAF_TYPE_2_ID<INC>::args_type>(PAF_TYPE_2_ID<INC>::id,      \
																	PAF_TYPE_2_ID<INC>::flag,    \
																	PAF_TO_STR(TYPE));
#define REG_FACTORY_OBJECT(INC, TYPE) PAF_REG_FACTORY_OBJECT_(INC, TYPE)

