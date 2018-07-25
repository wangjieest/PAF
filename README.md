#PAF Pluginable Abstract Factory

let's look at a simple usage 

I have a interface and it‘s implemented as below, with a method bar:

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


with PAF, we need only three line noninvasive code

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
    // Associate the interface with the corresponding implementation class
    REG_FACTORY_OBJECT(foo_i, foo);    

wherever needed (cross-source/cross-module), we just need to include the interface file for foo_i. (consistent with the interface usage scenario, no additional actions are required).

this is magic show time, which creates local objects with parameter. 

    // just include the interface file and use it as below
    // you need not know where the implementation file was, it will be in another module, or a pluginable module.
    #include "foo_i.h" 
    auto obj = CREATE_OBJECT(foo_i, 42); // std::shared_ptr<foo_i>
    printf("%d", obj->bar()); // 43

the whole process can also rely on the compiler to check for errors. objects created through factories are naturally smart pointers.

of course, it also supports the creation of singleton objects:

    // ParameterTypeList is a list of types for the interface implementation class constructor.
    REG_INTERFACE_SINGLTETON(InterfaceName, ParameterTypeList...)
    or 
    REG_INTERFACE_STATIC(InterfaceName, ParameterTypeList...)
    // thread-safe as static
    auto * inc = GET_SINGLETON(InterfaceName, ParameterTypeList...);
    // gets only
    auto * inc = GET_SINGLETON2(InterfaceName);
    // Destruction of the singleton
    DESTROY_SINGLETON(InterfaceName);

singleton will be destroyed manually or by factory with same order as statics. 
it is more flexible and controllable than the general static singleton.

in addition, a more secure pattern is implemented than a single case control: global shared objects. 
usually as parent of sub objects. to break circle reference and parameter passing to subs.

    REG_INTERFACE_SHARED(InterfaceName, ParameterTypeList...)
    // return a smart pointer
    // holding the interface smart pointer can ensure that the interface is not destroyed
    auto inc = GET_SHARED(InterfaceName, ParameterTypeList...); 
    // gets only
    auto * inc = GET_SHARED2(InterfaceName);
    // when all the smart pointers holding the interface are destroyed, 
    // the global shared object is destroyed automatically

in particular, global shared objects can be shared across modules, based on the base layer of the abstract factory. shared objects are destroyed only when all module references are destroyed.

based on the above features, you can also support elegant, leak-free exits. Perform leak and circular reference checks during release.
