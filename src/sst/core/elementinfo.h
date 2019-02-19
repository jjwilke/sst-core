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

class OldELITag {
 public:
  OldELITag(const std::string& elemlib) :
   lib(elemlib)
  {
  }

  std::string lib;
};

const std::vector<int> SST_ELI_VERSION = {0, 9, 0};

namespace ELI {

template <class Policy, class... Policies>
class FactoryInfo : public Policy, public FactoryInfo<Policies...>
{
  using Parent=FactoryInfo<Policies...>;
 public:
  template <class T> FactoryInfo(T* t) :
    Policy(t), Parent(t)
  {
  }

  template <class U> FactoryInfo(OldELITag& tag, U* u) :
    Policy(tag,u), Parent(tag,u)
  {
  }

  void toString(std::ostream& os) const {
    Parent::toString(os);
    Policy::toString(os);
  }
};

class FactoryInfoBase {
  friend class ModuleDocOldEli;
 public:
  const std::string getLibrary() const {
    return lib_;
  }
  const std::string getDescription() const {
    return desc_;
  }
  const std::string getName() const {
    return name_;
  }
  const std::vector<int>& getVersion() const {
    return version_;
  }
  const std::string getCompileFile() const {
    return file_;
  }
  const std::string getCompileDate() const {
    return date_;
  }
  const std::vector<int>& getELICompiledVersion() const {
    return compiled_;
  }
  std::string getELIVersionString() const;

 protected:
  template <class T> FactoryInfoBase(T* UNUSED(t)) :
   lib_(T::ELI_getLibrary()),
   name_(T::ELI_getName()),
   desc_(T::ELI_getDescription()),
   version_(T::ELI_getVersion()),
   file_(T::ELI_getCompileFile()),
   date_(T::ELI_getCompileDate()),
   compiled_(T::ELI_getELICompiledVersion())
  {
  }

  template <class U> FactoryInfoBase(OldELITag& tag, U* u) :
    lib_(tag.lib),
    name_(u->name),
    desc_(u->description),
    file_("UNKNOWN"),
    date_("UNKNOWN")
  {
  }

  void toString(std::ostream& os) const;

 private:
  std::string lib_;
  std::string name_;
  std::string desc_;
  std::vector<int> version_;
  std::string file_;
  std::string date_;
  std::vector<int> compiled_;
};

template <> class FactoryInfo<void> : public FactoryInfoBase {
 protected:
  template <class T> FactoryInfo(T* t) :
    FactoryInfoBase(t)
  {
  }

  template <class U> FactoryInfo(OldELITag& tag, U* u) :
    FactoryInfoBase(tag, u)
  {
  }

};

template <class Base, class... Args> struct FactoryCtorBase {
  virtual Base* create(Args... args) = 0;
};

#define ELI_RegisterCommon(Base) \
  template <class... __Args> \
  using Library = ::SST::ELI::ElementLibrary<Base,__Args...>; \
  template <class __TT, class... __Args> \
  using Instance = ::SST::ELI::FactoryInstance<Base,__TT,__Args...>;

#define ELI_RegisterBase(Base,...) \
  ELI_RegisterCommon(Base) \
  using Info = ::SST::ELI::FactoryInfo<__VA_ARGS__,void>;

#define ELI_RegisterBaseDefault(Base) \
  ELI_RegisterCommon(Base) \
  using Info = ::SST::ELI::FactoryInfo<void>;

#define ELI_RegisterCtor(Base,...) \
  template <class __TT> \
  using Factory = ::SST::ELI::FactoryInstance<Base,__TT,__VA_ARGS__>; \
  static ::SST::ELI::ElementLibrary<Base,__VA_ARGS__>* getLibrary(const std::string& name){ \
    return ::SST::ELI::ElementDatabase::getLibrary<Base,__VA_ARGS__>(name); \
  }

class ImplementsParamInfo {
 public:
   const std::vector<ElementInfoParam>& getValidParams() const {
     return params_;
   }

   std::string getParametersString() const;

   void toString(std::ostream& os) const {
     os << getParametersString() << "\n";
   }

   const Params::KeySet_t& getParamNames() const {
     return allowedKeys;
   }

 protected:
  template <class T> ImplementsParamInfo(T* UNUSED(t)) :
   params_(T::ELI_getParams()){
   init();
  }

  template <class U> ImplementsParamInfo(OldELITag& UNUSED(tag), U* u)
  {
     auto *p = u->params;
     while (NULL != p && NULL != p->name) {
      params_.emplace_back(*p);
      p++;
     }
     init();
  }

 private:
  void init(){
    for (auto& item : params_){
      allowedKeys.insert(item.name);
    }
  }

  Params::KeySet_t allowedKeys;
  std::vector<ElementInfoParam> params_;
};


class ImplementsStatsInfo {
 private:
  std::vector<std::string> statnames;
  std::vector<ElementInfoStatistic> stats_;

  void init(){
    for (auto& item : stats_) {
        statnames.push_back(item.name);
    }
  }

 protected:
  template <class T> ImplementsStatsInfo(T* UNUSED(t)) :
    stats_(T::ELI_getValidStats())
  {
    init();
  }

  template <class U> ImplementsStatsInfo(OldELITag& UNUSED(tag), U* u)
  {
    auto *s = u->stats;
    while ( NULL != s && NULL != s->name ) {
        stats_.emplace_back(*s);
        s++;
    }
    init();
  }

 public:
  const std::vector<ElementInfoStatistic>& getValidStats() const {
    return stats_;
  }
  const std::vector<std::string>& getStatnames() const { return statnames; }

  void toString(std::ostream& os) const {
    os << getStatisticsString() << "\n";
  }

  std::string getStatisticsString() const;
};


class ImplementsCategoryInfo {
 public:
  uint32_t category() const {
    return cat_;
  }

 protected:
  template <class T> ImplementsCategoryInfo(T* UNUSED(t)) :
    cat_(T::ELI_category())
  {
  }

  template <class U> ImplementsCategoryInfo(OldELITag UNUSED(tag), U* u) :
    cat_(u->category)
  {
  }

  void toString(std::ostream& UNUSED(os)) const {}

 private:
  uint32_t cat_;
};


class ImplementsPortsInfo {
 public:
  const std::vector<std::string>& getPortnames() { return portnames; }
  const std::vector<ElementInfoPort>& getValidPorts() const {
    return ports_;
  }

  std::string getPortsString() const;

  void toString(std::ostream& os) const {
    os << getPortsString() << "\n";
  }

 protected:
  template <class T> ImplementsPortsInfo(T* UNUSED(t)) :
    ports_(T::ELI_getPorts())
  {
    init();
  }

  template <class U> ImplementsPortsInfo(OldELITag& UNUSED(tag), U* u)
  {
    auto* po = u->ports;
    while ( NULL != po && NULL != po->name ) {
        portnames.emplace_back(po->name);
        po++;
    }
    init();
  }

 private:
  void init(){
    for (auto& item : ports_) {
      portnames.push_back(item.name);
    }
  }

  std::vector<std::string> portnames;
  std::vector<ElementInfoPort> ports_;

};

class ImplementsSubComponentInfo {
 public:
  const std::vector<ElementInfoSubComponentSlot>& getSubComponentSlots() const {
    return slots_;
  }

  std::string getSubComponentSlotString() const;

  void toString(std::ostream& os) const {
    os << getSubComponentSlotString() << "\n";
  }

 protected:
  template <class T> ImplementsSubComponentInfo(T* UNUSED(t)) :
    slots_(T::ELI_getSubcomponentSlots())
  {
  }

  template <class U> ImplementsSubComponentInfo(OldELITag& UNUSED(tag), U* u)
  {
    auto *s = u->subComponents;
    while ( NULL != s && NULL != s->name ) {
      slots_.emplace_back(*s);
      s++;
    }
  }

 private:
  std::vector<ElementInfoSubComponentSlot> slots_;
};

class ImplementsInterface {
 public:
  const std::string getInterface() const {
    return iface_;
  }

  void toString(std::ostream& os) const {
    os << "Interface: " << iface_ << "\n";
  }

 protected:
  template <class T> ImplementsInterface(T* UNUSED(t)) :
    iface_(T::ELI_getInterface())
  {
  }

  template <class U> ImplementsInterface(OldELITag& UNUSED(tag), U* u) :
    iface_(u->provides)
  {
  }

 private:
  std::string iface_;
};


//template <class... Args> Args... getDummyCtorValues();

template <class> struct wrap { typedef void type; };

/**
 * @class callCreate
 * ELI_customCreate is not a required method for any of the classes.
 * This uses SFINAE tricks to call on classes that implement
 * the method or avoid calling it on classes that don't implement it
 */
template<typename T, class... Args>
struct CallCreate {
 public:
  template <class... InArgs>
  T* operator()(InArgs&&... args){
    //if we compile here, class T does not have a custom create
    return new T(std::forward<InArgs>(args)...);
  }
};

/**
template<typename T, class... Args>
struct CallCreate<T, void, Args...>
{
 public:
  template <class... InArgs>
  T* operator()(InArgs&&... args){
    //if we compile here, class T does not have a custom create
    return new T(std::forward<InArgs>(args)...);
  }
};


template<typename T, class... Args>
struct CallCreate<T,
  typename wrap<
    decltype(std::declval<T>().ELI_customCreate(getDummyCtorValues<Args...>()))
   >::type, Args...
> : public std::true_type
{
 public:
  template <class... InArgs>
  T* operator()(InArgs&&... args){
    return T::ELI_customCreate(std::forward<InArgs>(args)...);
  }
};
*/

template <class Base, class... Args>
struct FactoryCtor
{
  virtual Base* create(Args... ctorArgs) = 0;
};

template <class Base, class T, class... Args>
struct CachedFactoryCtor : public FactoryCtor<Base,Args...>
{
  Base* create(Args... ctorArgs) override {
    if (!cached_){
      cached_ = CallCreate<T,Args...>(std::forward<Args>(ctorArgs)...);
    }
    return cached_;
  }

  static Base* cached_;
};

template <class Base, class T, class... Args>
struct FactoryCtorInstance : public FactoryCtor<Base,Args...>
{
  Base* create(Args... ctorArgs) override {
    // return new T(id, params);
    return CallCreate<T, Args...>()(std::forward<Args>(ctorArgs)...);
  }
};

template <class Base, class... Args>
struct FactoryCtorInstance<Base,OldELITag,Args...> :
  public FactoryCtorBase<Base,Args...>
{
 public:
  typedef Base* (*createFxn)(Args...);

  Base* create(Args... ctorArgs) override {
    return create_(std::forward<Args>(ctorArgs)...);
  }

 protected:
  FactoryCtorInstance(OldELITag UNUSED(tag), createFxn fxn) :
    create_(fxn)
  {
  }

 private:
  createFxn create_;
};

template <class Base, class... Args>
struct Factory :
  public FactoryCtor<Base,Args...>,
  public Base::Info
{
  template <class T> Factory(T* t) : Base::Info(t) {}

  template <class Info> Factory(OldELITag tag, Info* info) : Base::Info(tag,info) {}
};

template <class Base, class T, class... Args>
struct FactoryInstance :
  public FactoryCtorInstance<Base,T,Args...>,
  public Factory<Base,Args...>
{
  FactoryInstance() :
    FactoryCtorInstance<Base,T,Args...>(),
    Factory<Base,Args...>((T*)nullptr)
  {
  }

  Base* create(Args... ctorArgs) override {
    return FactoryCtorInstance<Base,T,Args...>::create(std::forward<Args>(ctorArgs)...);
  }

  template <class Fxn, class Info>
  FactoryInstance(OldELITag tag, Fxn fxn, Info* info) :
    FactoryCtorInstance<Base,OldELITag,Args...>(tag,fxn),
    Factory<Base,Args...>(tag, info)
  {
  }

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


template <class Base, class... CtorArgs>
class ElementLibrary
{
 public:
  using BaseFactory = Factory<Base,CtorArgs...>;
  BaseFactory* getFactory(const std::string &name) {
    auto iter = factories_.find(name);
    if (iter == factories_.end()){
      return nullptr;
    } else {
      return iter->second;
    }
  }

  static ElementLibrary* get(const std::string& name);

  void addFactory(BaseFactory* f){
    factories_[f->getName()] = f;
  }

 private:
  std::map<std::string, BaseFactory*> factories_;
};

template <class Base, class... CtorArgs>
class ElementLibraryDatabase {
 public:
  using Library=ElementLibrary<Base,CtorArgs...>;
  using BaseFactory=typename Library::BaseFactory;

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

 static bool addFactory(BaseFactory* factory){
   auto* info = getLibrary(factory->getLibrary());
   info->addFactory(factory);
   return true;
 }

 private:
  // Database - needs to be a pointer for static init order
  static std::unique_ptr<std::map<std::string,Library*>> libraries;
};
template <class Base, class... CtorArgs>
 std::unique_ptr<std::map<std::string,ElementLibrary<Base,CtorArgs...>*>>
  ElementLibraryDatabase<Base,CtorArgs...>::libraries;

template <class Base, class... Args>
ElementLibrary<Base,Args...>*
ElementLibrary<Base,Args...>::get(const std::string &name){
  return ElementLibraryDatabase<Base,Args...>::getLibrary(name);
}

template <class T> struct FactoryCtorInstance<SSTElementPythonModule,T> :
 public CachedFactoryCtor<SSTElementPythonModule,T>
{
};

template <class Base, class T, class... CtorArgs>
 const bool FactoryInstance<Base,T,CtorArgs...>::loaded =
  ElementLibraryDatabase<Base,CtorArgs...>::addFactory(new FactoryInstance<Base,T,CtorArgs...>);

struct ElementDatabase {
  template <class T, class... Args>
  static ElementLibrary<T,Args...>* getLibrary(const std::string& name){
    return ElementLibraryDatabase<T,Args...>::getLibrary(name);
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

#define ELI_FORWARD_AS_ONE(...) __VA_ARGS__

#define SST_ELI_INSERT_COMPILE_INFO() \
    static const std::string& ELI_getCompileDate() { \
        static std::string time = __TIME__; \
        static std::string date = __DATE__; \
        static std::string date_time = date + " " + time; \
        return date_time;                                 \
    } \
    static const std::string ELI_getCompileFile() { \
        return __FILE__; \
    } \
    static const std::vector<int>& ELI_getELICompiledVersion() { \
        static const std::vector<int> var(SST::SST_ELI_VERSION);      \
        return var; \
    }

#define SST_ELI_DEFAULT_INFO(lib,name,version,desc) \
  SST_ELI_INSERT_COMPILE_INFO() \
  static constexpr unsigned majorVersion() { \
    return SST::SST_ELI_getMajorNumberFromVersion(version); \
  } \
  static constexpr unsigned minorVersion() { \
    return SST::SST_ELI_getMinorNumberFromVersion(version); \
  } \
  static constexpr unsigned tertiaryVersion() { \
    return SST::SST_ELI_getTertiaryNumberFromVersion(version); \
  } \
  static const std::vector<int>& ELI_getVersion() { \
  static std::vector<int> var = version; \
      return var; \
  } \
  static const std::string ELI_getLibrary() { \
    return lib; \
  } \
  static const std::string ELI_getName() { \
    return name; \
  } \
  static const std::string ELI_getDescription() {  \
    return desc; \
  }

#define SST_ELI_CATEGORY_INFO(cat) \
  static const uint32_t ELI_getCategory() {  \
    return cat; \
  }

#define SST_ELI_INTERFACE_INFO(interface) \
  static const std::string ELI_getInterface() {  \
    return interface; \
  }

#define SST_ELI_REGISTER_COMPONENT(cls,lib,name,version,desc,cat)   \
    bool ELI_isLoaded() {                           \
        return SST::ComponentDoc<cls,SST::SST_ELI_getMajorNumberFromVersion(version),SST::SST_ELI_getMinorNumberFromVersion(version),SST::SST_ELI_getTertiaryNumberFromVersion(version)>::isLoaded(); \
    } \
    SST_ELI_CATEGORY_INFO(cat) \
    SST_ELI_DEFAULT_INFO(lib,name,desc) \
    SST_ELI_INSERT_COMPILE_INFO()

#define SST_ELI_ELEMENT_VERSION(...) {__VA_ARGS__}

#define SST_ELI_DOCUMENT_PARAMS(...)                              \
    static const std::vector<SST::ElementInfoParam>& ELI_getParams() { \
        static std::vector<SST::ElementInfoParam> var = { __VA_ARGS__ } ; \
        return var; \
    }


#define SST_ELI_DOCUMENT_STATISTICS(...)                                \
    static const std::vector<SST::ElementInfoStatistic>& ELI_getStatistics() {  \
        static std::vector<SST::ElementInfoStatistic> var = { __VA_ARGS__ } ;  \
        return var; \
    }


#define SST_ELI_DOCUMENT_PORTS(...)                              \
    static const std::vector<SST::ElementInfoPort2>& ELI_getPorts() { \
        static std::vector<SST::ElementInfoPort2> var = { __VA_ARGS__ } ;      \
        return var; \
    }

#define SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(...)                              \
    static const std::vector<SST::ElementInfoSubComponentSlot>& ELI_getSubComponentSlots() { \
    static std::vector<SST::ElementInfoSubComponentSlot> var = { __VA_ARGS__ } ; \
        return var; \
    }


#define SST_ELI_REGISTER_SUBCOMPONENT(cls,lib,name,version,desc,interface)   \
    bool ELI_isLoaded() {  \
      return SST::ELI::FactoryInstance<SST::SubComponent,cls>::isLoaded(); \
    } \
    SST_ELI_INTERFACE_INFO(interface) \
    SST_ELI_DEFAULT_INFO(lib,name,desc) \
    SST_ELI_INSERT_COMPILE_INFO()

#define SST_ELI_REGISTER_MODULE(cls,lib,name,version,desc,interface)    \
    bool ELI_isLoaded() {                           \
      return SST::ELI::FactoryInstance<SST::Module,cls>::isLoaded(); \
    } \
    SST_ELI_INTERFACE_INFO(interface) \
    SST_ELI_DEFAULT_INFO(lib,name,ELI_FORWARD_AS_ONE(version),desc)



#define SST_ELI_REGISTER_PARTITIONER(cls,lib,name,version,desc) \
    bool ELI_isLoaded() { \
        return SST::ELI::FactoryInstance<SST::Partition::SSTPartitioner, cls, \
                 SST::RankInfo, SST::RankInfo, int>::isLoaded(); \
    } \
    SST_ELI_DEFAULT_INFO(lib,name,ELI_FORWARD_AS_ONE(version),desc)


#define SST_ELI_REGISTER_PYTHON_MODULE(cls,lib,version)    \
    bool ELI_isLoaded() { \
        return SST::ELI::FactoryInstance<SST::SSTElementPythonModule,cls>::isLoaded(); \
    } \
    SST_ELI_DEFAULT_INFO(lib,name,ELI_FORWARD_AS_ONE(version),desc)

#define SST_ELI_REGISTER_CTOR(base,cls,lib,name,version,desc,...) \
  bool ELI_isLoaded() { \
      return SST::ELI::FactoryInstance<base,cls,__VA_ARGS__>::isLoaded(); \
  } \
  SST_ELI_DEFAULT_INFO(lib,name,ELI_FORWARD_AS_ONE(version),desc)

#define SST_ELI_REGISTER_DEFAULT_CTOR(base,cls,lib,name,version,desc) \
  bool ELI_isLoaded() { \
      return SST::ELI::FactoryInstance<base,cls>::isLoaded(); \
  } \
  SST_ELI_DEFAULT_INFO(lib,name,ELI_FORWARD_AS_ONE(version),desc)

} //namespace SST

#endif // SST_CORE_ELEMENTINFO_H
