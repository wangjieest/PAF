#pragma once
#include "PAFConfig.h"
#include <tuple>
namespace PAF_NAMESPACE
{
enum interface_flag : unsigned char
{
	create_as_local = 0x01,
	create_as_shared = 0x02,
	create_as_single = 0x04,
	create_as_static = 0x08, // cannot destroy by user
	create_with_args = 0x10, // construct with args
};

namespace detail
{
template <class F, class Tuple, std::size_t... I>
constexpr decltype(auto) apply_impl(F& f, Tuple&& t, std::index_sequence<I...>)
{
	return f(std::get<I>(std::forward<Tuple>(t))...);
}
template <class F, class Tuple>
constexpr decltype(auto) apply(F& f, Tuple&& t)
{
	return apply_impl(f,
					  std::forward<Tuple>(t),
					  std::make_index_sequence<std::tuple_size<typename std::decay<Tuple>::type>::value>{});
}
template <typename T>
using forward_t = typename std::conditional<std::is_reference<T>::value || std::is_arithmetic<T>::value, T, T&&>::type;
template <typename... Ts>
using typelist_t = std::tuple<Ts...>;

struct place_holder_t
{
	std::nullptr_t operator&()
	{
		return nullptr;
	}
};

} // namespace detail
} // namespace PAF_NAMESPACE
