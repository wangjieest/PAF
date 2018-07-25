#include "NZBase.h"
#include "PAFactory.h"
#include <thread>
#include <chrono>

namespace PAF_NAMESPACE
{
class paf_lock_t
{
public:
	paf_lock_t(std::atomic_bool& flag)
		: flag_(flag)
	{
	}
	~paf_lock_t() = default;
	PAF_DISABLE_COPY(paf_lock_t);
	void lock()
	{
		size_t back_off = 0;
		while (flag_.exchange(true, std::memory_order::memory_order_acquire))
		{
			wait_a_monment(back_off);
		}
	}
	void unlock()
	{
		flag_ = false;
	}
	static void wait_a_monment(size_t& back_off)
	{
		std::this_thread::yield();
	}

private:
	std::atomic_bool& flag_;
	static bool single_thread;
};
bool paf_lock_t::single_thread = (1 == std::thread::hardware_concurrency());

global_factory& inner_get_global_factory()
{
	static global_factory s_factory;
	return s_factory;
}

global_factory::global_factory()
{
}
global_factory::~global_factory()
{
}

void* global_factory::create_local(PAF_OBJ_ID id, void* data)
{
	void* ret = nullptr;
	do
	{
		auto meta = get_meta_info(id);
		if (!meta)
			break;

		if (!meta->exclusive())
		{
			if (meta->is_with_args() && !data)
			{
				PAF_ASSERT(!"must create_with_args");
				break;
			}

			meta->count_ += 1;
			ret = meta->create_obj_(data);
		}
		else
		{
			PAF_ASSERT(!"create_as_local not allowed");
		}
	} while (0);
	return ret;
}

void global_factory::destroy_local(PAF_OBJ_ID id, void* data)
{
	PAF_ASSERT(data);
	auto meta = get_meta_info(id);
	if (!meta)
		return;
	meta->count_ -= 1;
	meta->destroy_obj_(data);
}

void global_factory::release_all_global_objects()
{
	{
		std::lock_guard<std::mutex> lk(cs_);
		std::vector<PAF_OBJ_ID> tmp_order = std::move(obj_order_);
		for (auto& id : tmp_order)
		{
			if (id)
			{
				auto it = id_to_meta_.find(id);
				if (it != id_to_meta_.end())
				{
					auto meta = it->second.get();
					destroy_singleton(meta, meta->data_);
				}
			}
		}
	}
}

factory_meta_info_t* global_factory::get_meta_info(PAF_OBJ_ID id)
{
	std::lock_guard<std::mutex> lk(cs_);
	auto iter = id_to_meta_.find(id);
	if (iter == id_to_meta_.end())
	{
		PAF_ASSERT(!"id not registered");
		return nullptr;
	}
	return iter->second.get();
}

void* global_factory::get_or_create_shared(PAF_OBJ_ID id, void* data)
{
	void* ret = nullptr;
	do
	{
		auto meta = get_meta_info(id);
		if (!meta)
			break;
		if (meta->is_shared())
		{
			if (meta->is_with_args() && !data)
			{
				PAF_ASSERT(!"must create_with_args");
				break;
			}
			ret = get_shared_object(meta, data);
		}
		else
		{
			PAF_ASSERT(!"create_as_shared not allowed");
		}
	} while (0);
	return ret;
}

void* global_factory::get_shared(PAF_OBJ_ID id, bool addref)
{
	void* ret = nullptr;
	do
	{
		auto meta = get_meta_info(id);
		if (!meta)
			break;
		if (!meta->is_shared())
		{
			PAF_ASSERT(!"create_as_shared not allowed");
			break;
		}
		ret = meta->data_.load(std::memory_order::memory_order_relaxed);
		if (!addref)
			break;

		return get_shared_ref(meta);
	} while (0);
	return ret;
}

void global_factory::destroy_shared(PAF_OBJ_ID id, void* data)
{
	auto meta = get_meta_info(id);
	if (!meta)
		return;
	release_shared_object(meta, data);
}

void* global_factory::get_shared_object(factory_meta_info_t* method, void* data)
{
	PAF_ASSERT(method && (!method->is_with_args() || data));
	if (!method->create_obj_)
		return nullptr;
	void* tmp = nullptr;
	const auto ref = method->ref_.fetch_add(1);
	PAF_ASSERT(ref >= 0);
	if (ref == 0)
	{
		tmp = method->data_.exchange(nullptr, std::memory_order::memory_order_acq_rel);
		if (!tmp)
		{
			// 没抢到销毁权,重新创建
			tmp = method->create_obj_(data);
			++total_shared_count_;
			method->count_ += 1;
		}
		method->data_.store(tmp, std::memory_order::memory_order_release);
		method->busy_.store(false, std::memory_order::memory_order_release);
	}
	else
	{
		while (method->busy_.load(std::memory_order::memory_order_acquire))
		{
			// 处理和release线程竞态
		}

		size_t count = 0;
		tmp = method->data_.load(std::memory_order::memory_order_acquire);
		while (!tmp)
		{
			paf_lock_t::wait_a_monment(count);
			tmp = method->data_.load(std::memory_order::memory_order_acquire);
		};
	}
	return tmp;
}

void* global_factory::get_shared_ref(factory_meta_info_t* method)
{
	PAF_ASSERT(method);
	void* tmp = nullptr;
	for (;;)
	{
		auto ref = method->ref_.load(std::memory_order::memory_order_relaxed);
		if (ref <= 0)
		{
			break;
		}
		else if (method->ref_.compare_exchange_strong(ref, ref + 1))
		{
			while (method->busy_.load(std::memory_order::memory_order_acquire))
			{
				// 处理和dec线程竞态
			}

			do
			{
				// 和创建线程的竞态
				tmp = method->data_.load(std::memory_order::memory_order_acquire);
			} while (!tmp);
			break;
		}
	}
	return tmp;
}

void global_factory::release_shared_object(factory_meta_info_t* method, void* data)
{
	void* p = method->data_;
	PAF_ASSERT(p == data);
	method->busy_.store(true, std::memory_order::memory_order_acquire);
	const auto ref = method->ref_.fetch_sub(1);
	PAF_ASSERT(ref > 0);
	if (ref == 1)
	{
		// lock-free
		auto val = method->data_.exchange(nullptr, std::memory_order::memory_order_acq_rel);
		method->busy_.store(false, std::memory_order::memory_order_release);
		if (val)
		{
			method->destroy_obj_(val);
			--total_shared_count_;
			method->count_ -= 1;
		}
	}
	else
		method->busy_.store(false, std::memory_order::memory_order_release);
}

void* global_factory::get_or_create_singleton(PAF_OBJ_ID id, void* data)
{
	void* ret = nullptr;
	do
	{
		auto meta = get_meta_info(id);
		if (!meta)
			break;

		if (!meta->is_single())
		{
			PAF_ASSERT(!"create_as_single not allowed");
			break;
		}

		if (meta->is_with_args() && !data)
		{
			PAF_ASSERT(!"must create_with_args");
			break;
		}

		ret = meta->data_.load(std::memory_order::memory_order_acquire);
		if (ret == nullptr)
		{
			auto created = false;
			{
				paf_lock_t paf_lock(meta->busy_);
				std::lock_guard<paf_lock_t> lk(paf_lock);
				ret = meta->data_.load(std::memory_order::memory_order_relaxed);
				if (ret == nullptr)
				{
					ret = meta->create_obj_(data);
					created = true;
					meta->data_.store(ret, std::memory_order::memory_order_release);
				}
			}
			if (created)
			{
				std::lock_guard<std::mutex> lk(cs_);
				obj_order_.push_back(id);
				printf("singleton loaded %llu", id);
				++total_shared_count_;
			}
		}
	} while (0);
	return ret;
}

void* global_factory::get_singleton(PAF_OBJ_ID id)
{
	void* ret = nullptr;
	do
	{
		auto meta = get_meta_info(id);
		if (!meta)
			break;

		if (!meta->is_single())
		{
			PAF_ASSERT(!"create_as_single not allowed");
			break;
		}
		ret = meta->data_.load(std::memory_order::memory_order_relaxed);
	} while (0);
	return ret;
}

bool global_factory::destroy_singleton(PAF_OBJ_ID id, void* data)
{
	bool ret = false;
	auto meta = get_meta_info(id);
	if (!meta)
		return ret;

	if (meta->is_static())
	{
		assert(!"can't destroy static by user");
		return ret;
	}

	ret = destroy_singleton(meta, data);
	if (ret)
	{
		std::lock_guard<std::mutex> lk(cs_);
		for (auto iter = obj_order_.begin(); iter != obj_order_.end(); ++iter)
		{
			if (*iter == id)
			{
				*iter = 0;
				printf("singleton unloaded %llu", id);
			}
		}
	}
	return ret;
}

bool global_factory::destroy_singleton(factory_meta_info_t* meta, void* data)
{
	bool ret = false;
	if (!meta->is_single())
		return ret;
	if (data)
	{
		auto p = data;
		if (!meta->data_.compare_exchange_strong(p, nullptr))
		{
			PAF_ASSERT(!"not found");
			printf("singleton not found %llu", meta->obj_id_);
		}
	}
	else
	{
		data = meta->data_.exchange(nullptr, std::memory_order::memory_order_acq_rel);
		if (!data)
		{
			PAF_ASSERT(!"not found");
			printf("singleton not found %llu", meta->obj_id_);
		}
	}

	if (data)
	{
		meta->destroy_obj_(data);
		printf("singleton destroyed %llu", meta->obj_id_);
		--total_shared_count_;
		ret = true;
	}
	return ret;
}

int global_factory::register_global_object(object_register_info_t* arr, size_t count)
{
	int registered = 0;
	std::lock_guard<std::mutex> lk(cs_);
	for (size_t i = 0; i < count; ++i)
	{
		auto a = std::make_unique<factory_meta_info_t>(arr[i]);
		auto id = a->obj_id_;
		a->count_ = 0;
		PAF_ASSERT(a->create_obj_ && a->destroy_obj_ && " no construction");
		if (id_to_meta_.find(id) == id_to_meta_.end())
		{
			id_to_meta_[id] = std::move(a);
			++registered;
		}
		else
		{
			PAF_ASSERT(!" id conflicts");
		}
	}
	return registered;
}
bool global_factory::try_unregister_object(PAF_OBJ_ID obj_id)
{
	std::lock_guard<std::mutex> lk(cs_);
	auto it = id_to_meta_.find(obj_id);
	if (it != id_to_meta_.end())
	{
		auto& a = it->second;
		PAF_ASSERT(!a->count_);
		if (!a->count_)
		{
			holder_.push_back(std::move(a));
			id_to_meta_.erase(it);
		}
		else
		{
			return false;
		}
	}
	return true;
}
void global_factory::unregister_object(PAF_OBJ_ID obj_id)
{
	if (!try_unregister_object(obj_id))
	{
		PAF_ASSERT(0);
	}
}

namespace detail
{
extern "C" PAF_LINK_API void reg_object_info(PAF_NAMESPACE::object_register_info_t& info)
{
	PAF_NAMESPACE::inner_get_global_factory().register_global_object(&info, 1);
}
} // namespace detail

extern "C" PAF_LINK_API PAF_NAMESPACE::global_factory_i* PAF_GET_GLOBAL_FACTORY()
{
	return &PAF_NAMESPACE::inner_get_global_factory();
}
extern "C" PAF_LINK_API void PAF_DESTROY_GLOBAL_FACTORY()
{
	PAF_NAMESPACE::inner_get_global_factory().release_all_global_objects();
}

} // namespace PAF_NAMESPACE
