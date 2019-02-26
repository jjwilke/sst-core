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

public:
    SST_ELI_REGISTER_BASE_DEFAULT(SSTElementPythonModule)
    SST_ELI_REGISTER_CTOR(const std::string&)

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
    SSTElementPythonModuleOldELI(const std::string& lib, genPythonModuleFunction func) :
        SSTElementPythonModule(lib),
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

template <>
struct DerivedFactory<SSTElementPythonModule,SSTElementPythonModuleOldELI,const std::string&> :
  public Factory<SSTElementPythonModule,const std::string&>
{
  SSTElementPythonModule* create(const std::string& lib) override {
    return new SSTElementPythonModuleOldELI(lib, func_);
  }

  DerivedFactory(genPythonModuleFunction func) :
    func_(func)
  {}

  genPythonModuleFunction func_;
};

}
}

#define SST_ELI_REGISTER_PYTHON_MODULE(cls,lib,version)    \
  SST_ELI_REGISTER_DERIVED(SST::SSTElementPythonModule,cls,lib,lib,ELI_FORWARD_AS_ONE(version),desc)

#endif // SST_CORE_MODEL_ELEMENT_PYTHON_H
