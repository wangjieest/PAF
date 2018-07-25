#pragma once
#include "PAFactoryInc.h"
#include "InterfaceRegisterStruct.h"
#include <atomic>
#include <vector>
#include <map>
#include <list>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace PAF_NAMESPACE
{
#pragma pack(push, 4)
struct factory_meta_info_t : public object_register_info_t
{
	factory_meta_info_t(const object_register_info_t& rhs)
	{
		create_obj_ = rhs.create_obj_;
		destroy_obj_ = rhs.destroy_obj_;
		count_ = rhs.count_;
		obj_id_ = rhs.obj_id_;
		flag_ = rhs.flag_;
	}

	~factory_meta_info_t()
	{
	}

	bool exclusive() const
	{
		return 0 == (flag_ & (interface_flag::create_as_local));
	}
	bool is_shared() const
	{
		return exclusive() && (flag_ & interface_flag::create_as_shared);
	}
	bool is_single() const
	{
		return exclusive() && (flag_ & interface_flag::create_as_single);
	}
	bool is_static() const
	{
		return exclusive() && (flag_ & interface_flag::create_as_static);
	}
	bool is_with_args() const
	{
		return 0 != (flag_ & interface_flag::create_with_args);
	}

	factory_meta_info_t(const factory_meta_info_t&) = delete;
	factory_meta_info_t& operator=(const factory_meta_info_t&) = delete;

	std::atomic<void*> data_{nullptr};
	std::atomic<int> ref_{0};
	std::atomic_bool busy_{false};
};

class global_factory : public global_factory_i
{
	using fn_shared_obj_destroy = void (*)(PAF_OBJ_ID id, void*);

public:
	global_factory();
	virtual ~global_factory();
	PAF_DISABLE_COPY(global_factory)

	// local
	virtual void* create_local(PAF_OBJ_ID id, void* data = nullptr) override;
	virtual void destroy_local(PAF_OBJ_ID id, void* data) override;

	// shared
	virtual void* get_or_create_shared(PAF_OBJ_ID id, void* data = nullptr) override;
	virtual void* get_shared(PAF_OBJ_ID id, bool addref) override;
	virtual void destroy_shared(PAF_OBJ_ID id, void* data) override;

	// singleton
	virtual void* get_or_create_singleton(PAF_OBJ_ID id, void* data = nullptr) override;
	virtual void* get_singleton(PAF_OBJ_ID id) override;
	virtual bool destroy_singleton(PAF_OBJ_ID id, void* data) override;

public:
	// return actual number of registered
	int register_global_object(object_register_info_t* arr, size_t count);
	bool try_unregister_object(PAF_OBJ_ID obj_id);
	void unregister_object(PAF_OBJ_ID obj_id);
	void release_all_global_objects();
	bool destroy_singleton(factory_meta_info_t* method, void* data);

protected:
	factory_meta_info_t* get_meta_info(PAF_OBJ_ID id);
	void* get_shared_ref(factory_meta_info_t* method);
	void* get_shared_object(factory_meta_info_t* method, void* p);
	void release_shared_object(factory_meta_info_t* method, void* data);

	std::mutex cs_;
	std::unordered_map<PAF_OBJ_ID, std::unique_ptr<factory_meta_info_t>> id_to_meta_{1024};
	std::vector<std::unique_ptr<factory_meta_info_t>> holder_{1024};
	std::atomic<int32_t> total_shared_count_{0};
	// singleton managerment
	std::vector<PAF_OBJ_ID> obj_order_{1024};
};
#pragma pack(pop)
} // namespace PAF_NAMESPACE
