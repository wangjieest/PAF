#pragma once

#include "PAF/InterfaceDeclareStruct.h"
#define DECL_INTERFACE_LOCAL(...) PAF_DECL_INTERFACE_LOCAL_(__VA_ARGS__)
#define DECL_INTERFACE_SHARED(...) PAF_DECL_INTERFACE_SHARED_(__VA_ARGS__)
#define DECL_INTERFACE_SINGLETON(...) PAF_DECL_INTERFACE_SINGLETON_(__VA_ARGS__)
#define DECL_INTERFACE_STATIC(...) PAF_DECL_INTERFACE_STATIC_(__VA_ARGS__)

// declare
// --------
// factory

#include "PAF/PAFactoryInc.h"

#define CREATE_OBJECT(INC, ...) PAF_NAMESPACE::factory_helper::create_local<INC>(__VA_ARGS__)

#define GET_SHARED(INC, ...) PAF_NAMESPACE::factory_helper::get_shared<INC>(__VA_ARGS__)
#define GET_SHARED2(INC) PAF_NAMESPACE::factory_helper::get_shared_ref<INC>()

#define GET_SINGLETON(INC, ...) PAF_NAMESPACE::factory_helper::get_singleton<INC>(__VA_ARGS__)
#define GET_SINGLETON2(INC) PAF_NAMESPACE::factory_helper::get_singleton_ref<INC>()
#define DESTROY_SINGLETON(INC) PAF_NAMESPACE::factory_helper::destroy_singleton<INC>()

// self-register
#include "InterfaceRegister.h"
