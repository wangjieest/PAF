#pragma once
#include "PAFConfig.h"
#include "InterfaceDeclareStruct.h"
#define PAF_DISABLE_COPY(CLASS)   \
	CLASS(const CLASS&) = delete; \
	CLASS& operator=(const CLASS&) = delete;
#define PAF_DISABLE_MOVE(CLASS) \
	CLASS(CLASS&&) = delete;    \
	CLASS& operator=(CLASS&&) = delete;


#include <memory>
#include <cassert>

namespace PAF_NAMESPACE
{
class global_factory_i
{
	friend class factory_helper;
	// local
	virtual void* create_local(PAF_OBJ_ID id, void* data) = 0;
	virtual void destroy_local(PAF_OBJ_ID id, void* data) = 0;

	// shared
	virtual void* get_or_create_shared(PAF_OBJ_ID id, void* data) = 0;
	virtual void* get_shared(PAF_OBJ_ID id, bool addref) = 0;
	virtual void destroy_shared(PAF_OBJ_ID id, void* data) = 0;

	// singleton
	virtual void* get_or_create_singleton(PAF_OBJ_ID id, void* data) = 0;
	virtual void* get_singleton(PAF_OBJ_ID id) = 0;
	virtual bool destroy_singleton(PAF_OBJ_ID id, void* data) = 0;

	// global set/get
	//virtual void* set_global(PAF_OBJ_ID id) = 0;
	//virtual void* get_global(PAF_OBJ_ID id) = 0;
};

extern "C" PAF_LINK_API global_factory_i* PAF_GET_GLOBAL_FACTORY();
extern "C" PAF_LINK_API void PAF_DESTROY_GLOBAL_FACTORY();

class factory_helper
{
public:
	// local
	template <typename TYPE, typename... Args>
	static PAF_SHARED_PTR<TYPE> create_local(Args&&... args)
	{
		static_assert(
			((PAF_TYPE_2_ID<TYPE>::flag & PAF_NAMESPACE::create_with_args) && sizeof...(Args)) ||
				(!(PAF_TYPE_2_ID<TYPE>::flag & PAF_NAMESPACE::create_with_args) &&
				 !sizeof...(Args)),
			"construct args mismatch");
		static_assert(PAF_TYPE_2_ID<TYPE>::flag & PAF_NAMESPACE::create_as_local,
					  "can't create as local");
		auto inc = PAF_GET_GLOBAL_FACTORY();
		if (!inc)
			return nullptr;
		typename PAF_TYPE_2_ID<TYPE>::args_type data{std::forward<Args>(args)...};
		auto ret = (TYPE*)inc->create_local(PAF_TYPE_2_ID<TYPE>::id, &data);
		if (!ret)
			return nullptr;
		return PAF_SHARED_PTR<TYPE>(ret, [](void* data) {
			auto p = PAF_GET_GLOBAL_FACTORY();
			if (p)
				p->destroy_local(PAF_TYPE_2_ID<TYPE>::id, data);
		});
	}

	// shared
	template <typename TYPE, typename... Args>
	static PAF_SHARED_PTR<TYPE> get_shared(Args&&... args)
	{
		static_assert(
			((PAF_TYPE_2_ID<TYPE>::flag & PAF_NAMESPACE::create_with_args) && sizeof...(Args)) ||
				(!(PAF_TYPE_2_ID<TYPE>::flag & PAF_NAMESPACE::create_with_args) &&
				 !sizeof...(Args)),
			"construct args mismatch");
		static_assert(PAF_TYPE_2_ID<TYPE>::flag & PAF_NAMESPACE::create_as_shared,
					  "can't create as shared");
		auto inc = PAF_GET_GLOBAL_FACTORY();
		if (!inc)
			return nullptr;
		typename PAF_TYPE_2_ID<TYPE>::args_type data{std::forward<Args>(args)...};
		auto ret = (TYPE*)inc->get_or_create_shared(PAF_TYPE_2_ID<TYPE>::id, &data);
		if (!ret)
			return nullptr;
		return PAF_SHARED_PTR<TYPE>(ret, [](void* data) {
			auto p = PAF_GET_GLOBAL_FACTORY();
			if (p)
				p->destroy_shared(PAF_TYPE_2_ID<TYPE>::id, data);
		});
	}
	template <typename TYPE>
	static PAF_SHARED_PTR<TYPE> get_shared_ref()
	{
		static_assert(PAF_TYPE_2_ID<TYPE>::flag & PAF_NAMESPACE::create_as_shared,
					  "can't get as shared");
		auto inc = PAF_GET_GLOBAL_FACTORY();
		if (!inc)
			return nullptr;
		auto ret = (TYPE*)inc->get_shared(PAF_TYPE_2_ID<TYPE>::id, true);
		if (!ret)
			return nullptr;
		return PAF_SHARED_PTR<TYPE>(ret, [](void* data) {
			auto p = PAF_GET_GLOBAL_FACTORY();
			if (p)
				p->destroy_shared(PAF_TYPE_2_ID<TYPE>::id, data);
		});
	}
	template <typename TYPE>
	static TYPE* get_shared2_raw()
	{
		static_assert(PAF_TYPE_2_ID<TYPE>::flag & PAF_NAMESPACE::create_as_shared,
					  "can't get as shared");
		auto inc = PAF_GET_GLOBAL_FACTORY();
		if (!inc)
			return nullptr;
		return (TYPE*)inc->get_shared(PAF_TYPE_2_ID<TYPE>::id, false);
	}

	// singleton
	template <typename TYPE, typename... Args>
	static TYPE* get_singleton(Args&&... args)
	{
		static_assert(
			((PAF_TYPE_2_ID<TYPE>::flag & PAF_NAMESPACE::create_with_args) && sizeof...(Args)) ||
				(!(PAF_TYPE_2_ID<TYPE>::flag & PAF_NAMESPACE::create_with_args) &&
				 !sizeof...(Args)),
			"construct args mismatch");
		static_assert(PAF_TYPE_2_ID<TYPE>::flag & PAF_NAMESPACE::create_as_single,
					  "can't get as singleton");
		auto inc = PAF_GET_GLOBAL_FACTORY();
		if (!inc)
			return nullptr;
		typename PAF_TYPE_2_ID<TYPE>::args_type data{std::forward<Args>(args)...};
		return (TYPE*)inc->get_or_create_singleton(PAF_TYPE_2_ID<TYPE>::id, &data);
	}
	template <typename TYPE>
	static TYPE* get_singleton_ref()
	{
		static_assert(PAF_TYPE_2_ID<TYPE>::flag & PAF_NAMESPACE::create_as_single,
					  "can't get as singteton");
		auto inc = PAF_GET_GLOBAL_FACTORY();
		if (!inc)
			return nullptr;
		return (TYPE*)inc->get_singleton(PAF_TYPE_2_ID<TYPE>::id);
	}
	template <typename TYPE>
	static bool destroy_singleton()
	{
		static_assert(!(PAF_TYPE_2_ID<TYPE>::flag & PAF_NAMESPACE::create_as_static),
					  "can't destrory by user side");
		static_assert(PAF_TYPE_2_ID<TYPE>::flag & PAF_NAMESPACE::create_as_single,
					  "can't destrory as singteton");
		auto inc = PAF_GET_GLOBAL_FACTORY();
		if (!inc)
			return false;
		auto s = inc->get_singleton(PAF_TYPE_2_ID<TYPE>::id);
		return (TYPE*)inc->destroy_singleton(PAF_TYPE_2_ID<TYPE>::id, s);
	}
};

} // namespace PAF_NAMESPACE
