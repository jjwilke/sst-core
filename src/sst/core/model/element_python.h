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


#ifndef SST_CORE_MODEL_ELEMENT_PYTHON_H
#define SST_CORE_MODEL_ELEMENT_PYTHON_H

#include <vector>
#include <string>
#include <sst/core/elementinfo.h>

namespace SST {

typedef void* (*genPythonModuleFunction)(void);


/** Base class for python modules in element libraries
 */
class SSTElementPythonModule {

protected:
    std::string library;
    std::string pylibrary;
    std::string sstlibrary;
    char* primary_module;

    std::vector<std::pair<std::string,char*> > sub_modules;

    // Only needed for supporting the old ELI
    SSTElementPythonModule() {}
    
public:
    SST_ELI_REGISTER_BASE_DEFAULT(SSTElementPythonModule)
    SST_ELI_REGISTER_CTORS(
      ELI_CTOR(const std::string&),
      ELI_DEFAULT_CTOR()
    )

    virtual ~SSTElementPythonModule() {}
    
    SSTElementPythonModule(const std::string& library);

    void addPrimaryModule(char* file);
    void addSubModule(std::string name, char* file);

    virtual void* load();
};

// Class to use to support old ELI
class SSTElementPythonModuleOldELI : public SSTElementPythonModule {
private:
    genPythonModuleFunction func;
    
public:
    SSTElementPythonModuleOldELI(genPythonModuleFunction func) :
        SSTElementPythonModule(),
        func(func)
        {
        }

    void* load() override {
        return (*func)();
    }
};

namespace ELI {
template <class T> struct Allocator<SSTElementPythonModule,T> :
 public CachedAllocator<SSTElementPythonModule,T>
{
};

}
}

#define SST_ELI_REGISTER_PYTHON_MODULE(cls,lib,version)    \
  SST_ELI_REGISTER_DERIVED(SST::SSTElementPythonModule,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc)

#endif // SST_CORE_MODEL_ELEMENT_PYTHON_H
