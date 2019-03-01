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
      SST_ELI_DECLARE_BASE(Module)
      SST_ELI_DECLARE_INFO(
         ELI::ProvidesParams,
         ELI::ImplementsInterface
      )
      SST_ELI_DECLARE_CTORS(
         ELI_CTOR(SST::Params&),
         ELI_CTOR(Component*,SST::Params&)
      )

      Module() {}
      virtual ~Module() {}

      //by default, no params to return
      //static const std::vector<SST::ElementInfoParam>& ELI_getParams() {
      //    static std::vector<SST::ElementInfoParam> var{};
      //    return var;
      //}

    };



} //namespace SST

#define SST_ELI_REGISTER_MODULE(cls,lib,name,version,desc,interface)    \
  SST_ELI_REGISTER_DERIVED(SST::Module,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc) \
  SST_ELI_INTERFACE_INFO(interface)

#endif // SST_CORE_ACTION_H

