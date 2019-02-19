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
      ELI_RegisterBase(Module,
         ELI::ImplementsParamInfo,
         ELI::ImplementsInterface)

      Module() {}
      virtual ~Module() {}

      //by default, no params to return
      static const std::vector<SST::ElementInfoParam>& ELI_getParams() {
          static std::vector<SST::ElementInfoParam> var{};
          return var;
      }

    };
} //namespace SST

// BOOST_CLASS_EXPORT_KEY(SST::Module)

#endif // SST_CORE_ACTION_H
