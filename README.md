let's look at a simple usage 

I have a interface and it‘s implication，with a method bar：

    // interface file
    class foo_i {
    public:
      // no need virtual destructor
      virtual int bar() = 0;
    }
    
    // class file
    class foo : public foo_i {
    public:
      foo(int val):value(val){}
      virtual int bar() override {return ++value;}
      int value;
    }


with PAF , we need only three line noninvasive

    // interface file
    class foo_i {
    public:
      virtual int bar() = 0;
    }
    #include "interface_declare.h"    // paf interface & register
    DELC_INTERFACE_LOCAL(foo_i, int);  // Declare, LOCAL means only local var

    // class file
    class foo : public foo_i {
    public:
      foo(int val) :value(val) {}
      virtual int bar() override {return ++value;}
      int value;
    }
    REG_FACTORY_OBJECT(foo_i, foo);    // Associate the interface with the corresponding implementation class

Wherever needed (cross-source/cross-module), we just need to include the interface file for foo_i. (consistent with the interface usage scenario, no additional actions are required).

This is magic show time, which creates local objects with parameter.

    auto obj = CREATE_OBJECT(foo_i, 42); // std::shared_ptr<foo_i>
    printf("%d", obj->bar()); // 43

The whole process can also rely on the compiler to check for errors. Objects created through factories are naturally smart Pointers.

Of course, it also supports the creation of singleton objects:

    // parameter type list is a list of the parameter types of the interface implementation class constructor.
    REG_INTERFACE_SINGLTETON(InterfaceName, ParameterTypeList...)
    or 
    REG_INTERFACE_STATIC(InterfaceName, ParameterTypeList...)
    // Thread-safe
    auto * inc = GET_SINGLETON(InterfaceName, ParameterTypeList...);
    // Gets only
    auto * inc = GET_SINGLETON2(InterfaceName);
    // Destruction of the singleton
    DESTROY_SINGLETON(InterfaceName);

Singleton will be destroyed manually or by factory with same order as statics. 
It is more flexible and controllable than the general static singleton.

In addition, a more secure pattern is implemented than a single case control: global Shared objects.

    REG_INTERFACE_SHARED(InterfaceName, ParameterTypeList...)
    // Return a smart pointer, holding the interface smart pointer can ensure that the interface is not destroyed
    auto inc = GET_SHARED(InterfaceName, ParameterTypeList...); 
    // Gets only
    auto * inc = GET_SHARED2(InterfaceName);
    // When all the smart Pointers holding the interface are destroyed, the global Shared object is destroyed automatically

In particular, global Shared objects can be shared across modules, based on the base layer of the abstract factory. Shared objects are destroyed only when all module references are destroyed.

Based on the above features, you can also support elegant, leak-free exits. Perform leak and circular reference checks during release.
