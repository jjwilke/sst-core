// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_MODULE_H
#define SST_CORE_MODULE_H

#include <sst/core/elementinfo.h>

namespace SST {
  /**
     Module is a tag class used with the loadModule function.
   */
    class Module {

    public:
      SST_ELI_REGISTER_BASE(Module,
         ELI::ImplementsParamInfo,
         ELI::ImplementsInterface)

      SST_ELI_REGISTER_CTORS(
         ELI_CTOR(SST::Params&),
         ELI_CTOR(Component*,SST::Params&)
      )

      Module() {}
      virtual ~Module() {}

      //by default, no params to return
      static const std::vector<SST::ElementInfoParam>& ELI_getParams() {
          static std::vector<SST::ElementInfoParam> var{};
          return var;
      }

    };

/**
//because the old module interface supported two ctors for dissimilar objects
//I need SFINAE tricks here to enable_if the right version
namespace ELI {



template <>
struct Factory<Module> :
  public Module::Info,
  public FactoryCtor<Module,SST::Params&>,
  public FactoryCtor<Module,Component*,SST::Params&>
{
  template <class T> Factory(T* t) :
    Module::Info(t)
  {
  }

  template <class Info> Factory(OldELITag& tag, Info* info) :
    Module::Info(tag,info)
  {
  }
};

template <>
struct DerivedFactory<Module, OldELITag> :
  public Factory<Module>
{
  using WithCompFxn = FactoryCtor<Module,Component*,SST::Params&>::createFxn;
  using WithoutCompFxn = FactoryCtor<Module,SST::Params&>::createFxn;

  template <class Info>
  DerivedFactory(OldELITag tag, WithCompFxn wfxn, WithoutCompFxn wofxn, Info* info) :
    Factory<Module>(tag, info),
    withFxn_(wfxn),
    withoutFxn_(wofxn)
  {
  }

  Module* create(SST::Params& p) override {
    return withoutFxn_(p);
  }

  Module* create(Component* c, SST::Params& p) override {
    return withFxn_(c,p);
  }

 private:
  WithCompFxn withFxn_;
  WithoutCompFxn withoutFxn_;
};

//typename

template <class T>
struct Allocator<Module,T> {
  template <class Q=T>
  typename std::enable_if<std::is_constructible<T,SST::Params&>::value, Q*>::type
  operator()(SST::Params& p){
    return new T(p);
  }

  template <class Q=T>
  typename std::enable_if<std::is_constructible<T,Component*,SST::Params&>::value, Q*>::type
  operator()(Component* c, SST::Params& p){
    return new T(c,p);
  }

  template <class Q=T>
  typename std::enable_if<!std::is_constructible<T,SST::Params&>::value, Q*>::type
  operator()(SST::Params& p){
    return new T(p);
  }

  template <class Q=T>
  typename std::enable_if<!std::is_constructible<T,Component*,SST::Params&>::value, Q*>::type
  operator()(Component* c, SST::Params& p){
    return new T(c,p);
  }

};

template <class T>
struct DerivedFactory<Module,T> :
 public Factory<Module>
{
  DerivedFactory() : Factory<Module>((T*)nullptr){}

  Module* create(SST::Params& p) override {
    return Allocator<Module,T>()(p);
  }

  Module* create(Component* c, SST::Params& p) override {
    return Allocator<Module,T>()(c,p);
  }
};

}
*/


} //namespace SST

// BOOST_CLASS_EXPORT_KEY(SST::Module)

#endif // SST_CORE_ACTION_H
