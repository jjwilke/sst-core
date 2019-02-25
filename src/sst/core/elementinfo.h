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

#ifndef SST_CORE_ELEMENTINFO_H
#define SST_CORE_ELEMENTINFO_H

#include <sst/core/sst_types.h>
#include <sst/core/warnmacros.h>
#include <sst/core/params.h>
#include <sst/core/statapi/statfieldinfo.h>

#include <sst/core/paramsInfo.h>
#include <sst/core/statsInfo.h>
#include <sst/core/defaultInfo.h>
#include <sst/core/portsInfo.h>
#include <sst/core/subcompSlotInfo.h>
#include <sst/core/interfaceInfo.h>
#include <sst/core/categoryInfo.h>
#include <sst/core/elementfactory.h>

#include <string>
#include <vector>

#include <sst/core/elibase.h>

namespace SST {
class Component;
class Module;
class SubComponent;
class BaseComponent;
namespace Partition {
    class SSTPartitioner;
}
namespace Statistics {
  template <class T, class... CtorArgs> class Statistic;
  class StatisticBase;
}
class RankInfo;
class SSTElementPythonModule;

/****************************************************
   Base classes for templated documentation classes
*****************************************************/


const std::vector<int> SST_ELI_VERSION = {0, 9, 0};

namespace ELI {

template <class Policy, class... Policies>
class FactoryInfoImpl : public Policy, public FactoryInfoImpl<Policies...>
{
  using Parent=FactoryInfoImpl<Policies...>;
 public:
  template <class... Args> FactoryInfoImpl(Args&&... args)
    : Policy(args...), Parent(args...) //forward as l-values
  {
  }

  template <class XMLNode> void outputXML(XMLNode* node){
    Policy::outputXML(node);
    Parent::outputXML(node);
  }

  void toString(std::ostream& os) const {
    Parent::toString(os);
    Policy::toString(os);
  }
};

template <> class FactoryInfoImpl<void> {
 protected:
  template <class... Args> FactoryInfoImpl(Args&&... UNUSED(args))
  {
  }

  template <class XMLNode> void outputXML(XMLNode* UNUSED(node)){}

  void toString(std::ostream& UNUSED(os)) const {}

};

template <class Base>
struct FactoryInfo : public Base::Info
{
  template <class T> FactoryInfo(T* t) :
    Base::Info(t)
  {
  }

  template <class Info> FactoryInfo(OldELITag& tag, Info* info) :
    Base::Info(tag,info)
  {
  }

};


template <class Base, class T>
struct DerivedFactoryInfo : public FactoryInfo<Base>
{
  DerivedFactoryInfo() : FactoryInfo<Base>((T*)nullptr)
  {
  }

};

template <class Base>
struct DerivedFactoryInfo<Base,OldELITag> :
  public Base::Info
{
  template <class Info>
  DerivedFactoryInfo(const std::string& library, Info* info) :
    Base::Info(OldELITag{library}, info)
  {
  }

};

template <class Base, class T>
struct InstantiateInfo {
  static bool isLoaded() {
    return loaded;
  }

  static const bool loaded;
};

class DataBase {
 public:
  static void addLoaded(const std::string& name){
    loaded_.insert(name);
  }

  static bool isLoaded(const std::string& name){
    return loaded_.find(name) != loaded_.end();
  }

 private:
  static std::set<std::string> loaded_;
};

template <class Base> class InfoLibrary
{
 public:
  using BaseInfo = typename Base::Info;

  BaseInfo* getInfo(const std::string& name) {
    auto iter = infos_.find(name);
    if (iter == infos_.end()){
      return nullptr;
    } else {
      return iter->second;
    }
  }

  int numEntries() const {
    return infos_.size();
  }

  const std::map<std::string, BaseInfo*> getMap() const {
    return infos_;
  }

  void addInfo(BaseInfo* info){
    infos_[info->getName()] = info;
  }

 private:
  std::map<std::string, BaseInfo*> infos_;
};


template <class Base>
class InfoLibraryDatabase {
 public:
  using Library=InfoLibrary<Base>;
  using BaseInfo=typename Library::BaseInfo;

  static Library* getLibrary(const std::string& name){
    if (!libraries){
      libraries = std::make_unique<std::map<std::string,Library*>>();
    }
    auto iter = libraries->find(name);
    if (iter == libraries->end()){
      DataBase::addLoaded(name);
      auto* info = new Library;
      (*libraries)[name] = info;
      return info;
    } else {
      return iter->second;
    }
  }

 private:
  // Database - needs to be a pointer for static init order
  static std::unique_ptr<std::map<std::string,Library*>> libraries;
};

template <class Base>
 std::unique_ptr<std::map<std::string,InfoLibrary<Base>*>>
  InfoLibraryDatabase<Base>::libraries;

template <class Base>
struct ElementsInfo
{
  static InfoLibrary<Base>* getLibrary(const std::string& name){
    return InfoLibraryDatabase<Base>::getLibrary(name);
  }

  template <class T> static bool add(){
    auto* info = new DerivedFactoryInfo<Base,T>();
    ElementsInfo<Base>::getLibrary(T::ELI_getLibrary())->addInfo(info);
    return true;
  }

};

template <class Base, class T>
 const bool InstantiateInfo<Base,T>::loaded = ElementsInfo<Base>::template add<T>();


struct InfoDatabase {
  template <class T>
  static InfoLibrary<T>* getLibrary(const std::string& name){
    return InfoLibraryDatabase<T>::getLibrary(name);
  }
};

} //namespace ELI


/**************************************************************************
  Class and constexpr functions to extract integers from version number.
**************************************************************************/

struct SST_ELI_element_version_extraction {
    const unsigned major;
    const unsigned minor;
    const unsigned tertiary;

    constexpr unsigned getMajor() { return major; }
    constexpr unsigned getMinor() { return minor; }
    constexpr unsigned getTertiary() { return tertiary; }
};

constexpr unsigned SST_ELI_getMajorNumberFromVersion(SST_ELI_element_version_extraction ver) {
    return ver.getMajor();
}

constexpr unsigned SST_ELI_getMinorNumberFromVersion(SST_ELI_element_version_extraction ver) {
    return ver.getMinor();
}

constexpr unsigned SST_ELI_getTertiaryNumberFromVersion(SST_ELI_element_version_extraction ver) {
    return ver.getTertiary();
}


/**************************************************************************
  Macros used by elements to add element documentation
**************************************************************************/



#define SST_ELI_REGISTER_COMMON(Base) \
  using __LocalEliName = Base; \
  template <class... __Args> \
  using FactoryLibrary = ::SST::ELI::FactoryLibrary<Base,__Args...>; \
  using InfoLibrary = ::SST::ELI::InfoLibrary<Base>; \
  template <class __TT> \
  using InfoInstance = ::SST::ELI::DerivedFactoryInfo<Base,__TT>; \
  static InfoLibrary* getInfoLibrary(const std::string& name){ \
    return SST::ELI::InfoDatabase::getLibrary<Base>(name); \
  } \
  static __LocalEliName::Info* getInfo(const std::string& elemlib, const std::string& name){ \
    auto* lib = getInfoLibrary(elemlib); \
    if (lib) return lib->getInfo(name); \
    else return nullptr; \
  } \
  static const char* ELI_baseName(){ return #Base; }



#define SST_ELI_REGISTER_BASE(Base,...) \
  using Info = ::SST::ELI::FactoryInfoImpl<__VA_ARGS__,SST::ELI::DefaultFactoryInfo,void>; \
  SST_ELI_REGISTER_COMMON(Base)

#define SST_ELI_REGISTER_BASE_DEFAULT(Base) \
  using Info = ::SST::ELI::FactoryInfoImpl<SST::ELI::DefaultFactoryInfo,void>; \
  SST_ELI_REGISTER_COMMON(Base)

#define SST_ELI_REGISTER_DERIVED(base,cls,lib,name,version,desc) \
  bool ELI_isLoaded() { \
      return SST::ELI::InstantiateFactory<base,cls>::isLoaded() \
        && SST::ELI::InstantiateInfo<base,cls>::isLoaded(); \
  } \
  SST_ELI_DEFAULT_INFO(lib,name,ELI_FORWARD_AS_ONE(version),desc)


} //namespace SST

#endif // SST_CORE_ELEMENTINFO_H
