#pragma once
#include "PAFConfig.h"
#include "TypeTraits.h"

#define PAF_EXPAND_(...) __VA_ARGS__
#define PAF_MACRO_CONCAT_(A, B) A##B
#define PAF_CONCAT(A, B) PAF_MACRO_CONCAT_(A, B)

// clang-format off
#define PAF_ARG_SEQ()    1,  1,  1,  1,  1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 0
#define PAF_ARG_PICK_N(_15,_14,_13,_12,_11,_10,_9,_8,_7,_6,_5,_4,_3,_2,_1, N, ...) N
#define PAF_ARG_SELECT_(...) PAF_EXPAND_(PAF_ARG_PICK_N( __VA_ARGS__))
#define PAF_ARG_SELECT(...) PAF_ARG_SELECT_(__VA_ARGS__, PAF_ARG_SEQ())
// clang-format on

const auto t0 = PAF_ARG_SELECT(NONE);
static_assert(t0 == 0, "Error 0");
const auto t1 = PAF_ARG_SELECT(NONE, 1);
static_assert(t1 == 1, "Error 1");
const auto t2 = PAF_ARG_SELECT(NONE, 2, 3);
static_assert(t2 == 1, "Error 2");

template <PAF_OBJ_ID ID>
struct PAF_ID_2_TYPE;
template <typename TYPE>
struct PAF_TYPE_2_ID;

#define PAF_DECL_INTERFACE_0(FLAG, INC)                                      \
	template <>                                                              \
	struct PAF_TYPE_2_ID<INC>                                                \
	{                                                                        \
		using type = INC;                                                    \
		using args_type = PAF_NAMESPACE::detail::place_holder_t;             \
		enum : PAF_OBJ_ID                                                    \
		{                                                                    \
			id = PAF_COMPILE_TIME_HASH(INC)                                  \
		};                                                                   \
		enum : std::underlying_type_t<PAF_NAMESPACE::interface_flag>         \
		{                                                                    \
			flag = FLAG                                                      \
		};                                                                   \
	};                                                                       \
	template <>                                                              \
	struct PAF_ID_2_TYPE<PAF_TYPE_2_ID<INC>::id> : public PAF_TYPE_2_ID<INC> \
	{                                                                        \
	};
#define PAF_DECL_INTERFACE_1(FLAG, INC, ...)                                 \
	template <>                                                              \
	struct PAF_TYPE_2_ID<INC>                                                \
	{                                                                        \
		using type = INC;                                                    \
		using args_type = PAF_NAMESPACE::detail::typelist_t<__VA_ARGS__>;    \
		enum : PAF_OBJ_ID                                                    \
		{                                                                    \
			id = PAF_COMPILE_TIME_HASH(INC)                                  \
		};                                                                   \
		enum : std::underlying_type_t<PAF_NAMESPACE::interface_flag>         \
		{                                                                    \
			flag = FLAG | PAF_NAMESPACE::create_with_args                    \
		};                                                                   \
	};                                                                       \
	template <>                                                              \
	struct PAF_ID_2_TYPE<PAF_TYPE_2_ID<INC>::id> : public PAF_TYPE_2_ID<INC> \
	{                                                                        \
	};

// allowed only local
#define PAF_DECL_INTERFACE_LOCAL_(...)          \
	PAF_EXPAND_(PAF_CONCAT(PAF_DECL_INTERFACE_, \
						   PAF_ARG_SELECT(__VA_ARGS__))(PAF_NAMESPACE::create_as_local, __VA_ARGS__))

// allowed only shared
#define PAF_DECL_INTERFACE_SHARED_(...)         \
	PAF_EXPAND_(PAF_CONCAT(PAF_DECL_INTERFACE_, \
						   PAF_ARG_SELECT(__VA_ARGS__))(PAF_NAMESPACE::create_as_shared, __VA_ARGS__))

// allowed only singleton
#define PAF_DECL_INTERFACE_SINGLETON_(...)      \
	PAF_EXPAND_(PAF_CONCAT(PAF_DECL_INTERFACE_, \
						   PAF_ARG_SELECT(__VA_ARGS__))(PAF_NAMESPACE::create_as_single, __VA_ARGS__))

// allowed only singleton
#define PAF_DECL_INTERFACE_STATIC_(...)                                                      \
	PAF_EXPAND_(PAF_CONCAT(PAF_DECL_INTERFACE_,                                              \
						   PAF_ARG_SELECT(__VA_ARGS__))(PAF_NAMESPACE::create_as_static |    \
															PAF_NAMESPACE::create_as_single, \
														__VA_ARGS__))
