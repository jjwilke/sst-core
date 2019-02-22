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

template <class> struct Wrap { typedef void type; };

template <class Policy, class... Policies>
class FactoryInfoImpl : public Policy, public FactoryInfoImpl<Policies...>
{
  using Parent=FactoryInfoImpl<Policies...>;
 public:
  template <class T> FactoryInfoImpl(T* t) :
    Policy(t), Parent(t)
  {
  }

  template <class U> FactoryInfoImpl(OldELITag&& tag, U* u) :
    Policy(tag,u), Parent(tag,u)
  {
  }

  template <class U> FactoryInfoImpl(OldELITag& tag, U* u) :
    Policy(tag,u), Parent(tag,u)
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

  void toString(std::ostream& os) const;

  template <class XMLNode> void outputXML(XMLNode* node) const {
    node->SetAttribute("Name", getName().c_str());
    node->SetAttribute("Description", getDescription().c_str());
  }


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

  template <class U> FactoryInfoBase(OldELITag&& tag, U* u) :
    FactoryInfoBase(tag,u)
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

 private:
  std::string lib_;
  std::string name_;
  std::string desc_;
  std::vector<int> version_;
  std::string file_;
  std::string date_;
  std::vector<int> compiled_;
};

template <> class FactoryInfoImpl<void> : public FactoryInfoBase {
 protected:
  template <class T> FactoryInfoImpl(T* t) :
    FactoryInfoBase(t)
  {
  }

  template <class U> FactoryInfoImpl(OldELITag& tag, U* u) :
    FactoryInfoBase(tag, u)
  {
  }

  template <class U> FactoryInfoImpl(OldELITag&& tag, U* u) :
    FactoryInfoBase(tag, u)
  {
  }

};

class ImplementsParamInfo {
 public:
   const std::vector<ElementInfoParam>& getValidParams() const {
     return params_;
   }

   std::string getParametersString() const;

   void toString(std::ostream& os) const {
     os << getParametersString() << "\n";
   }

   template <class XMLNode> void outputXML(XMLNode* node) const {
     // Build the Element to Represent the Component
     int idx = 0;
     for (const ElementInfoParam& param : params_){;
       // Build the Element to Represent the Parameter
       auto* XMLParameterElement = new XMLNode("Parameter");
       XMLParameterElement->SetAttribute("Index", idx);
       XMLParameterElement->SetAttribute("Name", param.name);
       XMLParameterElement->SetAttribute("Description", param.description);
       if (param.defaultValue) XMLParameterElement->SetAttribute("Default", param.defaultValue);
       node->LinkEndChild(XMLParameterElement);
       ++idx;
     }
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

  template <class XMLNode> void outputXML(XMLNode* node) const {
    // Build the Element to Represent the Component
    int idx = 0;
    for (const ElementInfoStatistic& stat : stats_){
      // Build the Element to Represent the Parameter
      auto* XMLStatElement = new XMLNode("Statistic");
      XMLStatElement->SetAttribute("Index", idx);
      XMLStatElement->SetAttribute("Name", stat.name);
      XMLStatElement->SetAttribute("Description", stat.description);
      if (stat.units) XMLStatElement->SetAttribute("Units", stat.units);
      XMLStatElement->SetAttribute("EnableLevel", stat.enableLevel);
      node->LinkEndChild(XMLStatElement);
      ++idx;
    }
  }

  std::string getStatisticsString() const;
};


class ImplementsCategoryInfo {
 public:
  uint32_t category() const {
    return cat_;
  }

  static const char* categoryName(int cat){
    switch(cat){
    case COMPONENT_CATEGORY_PROCESSOR:
      return "PROCESSOR COMPONENT";
    case COMPONENT_CATEGORY_MEMORY:
      return "MEMORY COMPONENT";
    case COMPONENT_CATEGORY_NETWORK:
      return "NETWORK COMPONENT";
    case COMPONENT_CATEGORY_SYSTEM:
      return "SYSTEM COMPONENT";
    default:
      return "UNCATEGORIZED COMPONENT";
    }
  }

  void toString(std::ostream& UNUSED(os)) const {
    os << "CATEGORY: " << categoryName(cat_) << "\n";
  }

  template <class XMLNode> void outputXML(XMLNode* UNUSED(node)){
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

  template <class XMLNode> void outputXML(XMLNode* node) const {
    // Build the Element to Represent the Component
    int idx = 0;
    for (const ElementInfoPort& port : ports_){
      auto* XMLPortElement = new XMLNode("Port");
      XMLPortElement->SetAttribute("Index", idx);
      XMLPortElement->SetAttribute("Name", port.name);
      XMLPortElement->SetAttribute("Description", port.description);
      node->LinkEndChild(XMLPortElement);
      ++idx;
    }
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

  template <class XMLNode> void outputXML(XMLNode* node) const {
    int idx = 0;
    for (const ElementInfoSubComponentSlot& slot : slots_){
      auto* element = new XMLNode("SubComponentSlot");
      element->SetAttribute("Index", idx);
      element->SetAttribute("Name", slot.name);
      element->SetAttribute("Description", slot.description);
      element->SetAttribute("Interface", slot.superclass);
      node->LinkEndChild(element);
      ++idx;
    }
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

  template <class XMLNode> void outputXML(XMLNode* node) const {
    node->SetAttribute("Interface", iface_.c_str());
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

template <class Base, class T, class... Args>
struct CachedAllocator
{
  Base* operator()(Args... ctorArgs) override {
    if (!cached_){
      cached_ = new T(std::forward<Args>(ctorArgs)...);
    }
    return cached_;
  }

  static Base* cached_;
};
template <class Base, class T, class... Args>
  Base* CachedAllocator<Base,T,Args...>::cached_ = nullptr;


template <class Base, class... Args>
struct Factory
{
  typedef Base* (*createFxn)(Args...);

  virtual Base* create(Args... ctorArgs) = 0;
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

template <class Base, class T, class... Args>
struct Allocator
{
  T* operator()(Args... args){
    return new T(std::forward<Args>(args)...);
  }
};


template <class Base, class T, class... Args>
struct DerivedFactory : public Factory<Base,Args...>
{
  Base* create(Args... ctorArgs) override {
    return Allocator<Base,T,Args...>()(std::forward<Args>(ctorArgs)...);
  }
};

template <class Base, class... Args>
struct DerivedFactory<Base,OldELITag,Args...> :
  public Factory<Base,Args...>
{
  using typename Factory<Base,Args...>::createFxn;

  DerivedFactory(createFxn fxn) :
    create_(fxn)
  {
  }

  Base* create(Args... ctorArgs) override {
    return create_(std::forward<Args>(ctorArgs)...);
  }

 private:
  createFxn create_;

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
struct Instantiate {
  static bool isLoaded() {
    return loaded;
  }

  static const bool loaded;
};

template <class Base, class T, class CtorTuple>
struct InstantiateCustomCtor {
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

template <class Base, class... CtorArgs>
class FactoryLibrary
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

  const std::map<std::string, BaseFactory*>& getMap() const {
    return factories_;
  }

  void addFactory(const std::string& name, BaseFactory* fact){
    factories_[name] = fact;
  }

 private:
  std::map<std::string, BaseFactory*> factories_;
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

template <class Base, class... CtorArgs>
class FactoryLibraryDatabase {
 public:
  using Library=FactoryLibrary<Base,CtorArgs...>;
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

 private:
  // Database - needs to be a pointer for static init order
  static std::unique_ptr<std::map<std::string,Library*>> libraries;
};

template <class Base, class... CtorArgs>
 std::unique_ptr<std::map<std::string,FactoryLibrary<Base,CtorArgs...>*>>
  FactoryLibraryDatabase<Base,CtorArgs...>::libraries;

template <class Base>
 std::unique_ptr<std::map<std::string,InfoLibrary<Base>*>>
  InfoLibraryDatabase<Base>::libraries;

template <class T, class U>
struct is_tuple_constructible : public std::false_type {};

template <class T, class... Args>
struct is_tuple_constructible<T, std::tuple<Args...>> :
  public std::is_constructible<T, Args...>
{
};

template <class Base, class CtorTuple>
struct ElementsFactory {};

template <class Base, class... Args>
struct ElementsFactory<Base, std::tuple<Args...>>
{
  static FactoryLibrary<Base,Args...>* getLibrary(const std::string& name){
    return FactoryLibraryDatabase<Base,Args...>::getLibrary(name);
  }

  template <class T> static Factory<Base,Args...>* makeFactory(){
    return new DerivedFactory<Base,T,Args...>();
  }

};

template <class Base>
struct ElementsInfo
{
  static InfoLibrary<Base>* getLibrary(const std::string& name){
    return InfoLibraryDatabase<Base>::getLibrary(name);
  }

  template <class T> static FactoryInfo<Base>* makeInfo(){
    return new DerivedFactoryInfo<Base,T>();
  }

};

template <class Base, class Ctor, class... Ctors>
struct CtorList : public CtorList<Base,Ctors...>
{
  template <class T, class U=T>
  static typename std::enable_if<is_tuple_constructible<U,Ctor>::value, bool>::type
  add(){
    auto* fact = ElementsFactory<Base,Ctor>::template makeFactory<U>();
    auto* info = ElementsInfo<Base>::template makeInfo<U>();
    ElementsInfo<Base>::getLibrary(T::ELI_getLibrary())->addInfo(info);
    ElementsFactory<Base,Ctor>::getLibrary(T::ELI_getLibrary())->addFactory(T::ELI_getName(), fact);
    return CtorList<Base,Ctors...>::template add<T>();
  }

  template <class T, class U=T>
  static typename std::enable_if<!is_tuple_constructible<U,Ctor>::value, bool>::type
  add(){
    return CtorList<Base,Ctors...>::template add<T>();
  }

  static typename Base::Info* getInfo(const std::string& elemlib, const std::string& name){
    auto* lib = ElementsInfo<Base>::getLibrary(elemlib);
    if (lib){
      return lib->getInfo(name);
    } else {
      return nullptr;
    }
  }

  //Ctor is a tuple
  template <class... InArgs>
  typename std::enable_if<std::is_convertible<std::tuple<InArgs&&...>, Ctor>::value, Base*>::type
  operator()(const std::string& elemlib, const std::string& name, InArgs&&... args){
    auto* lib = ElementsFactory<Base,Ctor>::getLibrary(elemlib);
    if (lib){
      auto* fact = lib->getFactory(name);
      if (fact){
        return fact->create(std::forward<InArgs>(args)...);
      }
      std::cerr << "For library " << elemlib << ", " << Base::ELI_baseName()
               << " " << name << " does not provide matching constructor"
               << std::endl;
    }
    std::cerr << "No matching constructors found in library " << elemlib
              << " for base " << Base::ELI_baseName() << std::endl;
    abort();
  }

  //Ctor is a tuple
  template <class... InArgs>
  typename std::enable_if<!std::is_convertible<std::tuple<InArgs&&...>, Ctor>::value, Base*>::type
  operator()(const std::string& elemlib, const std::string& name, InArgs&&... args){
    //I don't match - pass it up
    return CtorList<Base,Ctors...>::operator()(elemlib, name, std::forward<InArgs>(args)...);
  }

};



template <class T, class Enable=void>
struct normalize_arg { using type=T; };

template <class T>
struct normalize_arg<T*&,void> { using type=T*; };

template <class T> struct normalize_arg<T,
  typename std::enable_if<std::is_integral<typename  std::remove_reference<T>::type>::value, void>::type>
{
  using type=int64_t;
};

template <class T>
struct normalize_arg<T,
  typename std::enable_if<std::is_floating_point<typename std::remove_reference<T>::type>::value, void>::type>
{
  using type=double;
};

template <typename... T>
using tuple_with_normalized_args = std::tuple<typename normalize_arg<T>::type...>;

template <class T> struct normalized_tuple {};
template <class... Args> struct normalized_tuple<std::tuple<Args...>> {
  using type = typename tuple_with_normalized_args<Args...>::type;
};

template <class Base> struct CtorList<Base,void>
{
  template <class T> static bool add(){
    return true;
  }
};

template <class T> struct Allocator<SSTElementPythonModule,T> :
 public CachedAllocator<SSTElementPythonModule,T>
{
};

template <class Base, class T>
 const bool Instantiate<Base,T>::loaded = Base::Ctor::template add<T>();

template <class Base, class T, class CtorTuple>
 const bool InstantiateCustomCtor<Base,T,CtorTuple>::loaded
   = CtorList<Base,void>::template addCustomCtor<T,CtorTuple>();

struct FactoryDatabase {
  template <class T, class... Args>
  static FactoryLibrary<T,Args...>* getLibrary(const std::string& name){
    return FactoryLibraryDatabase<T,Args...>::getLibrary(name);
  }
};

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

#define ELI_FORWARD_AS_ONE(...) __VA_ARGS__

#define ELI_CTOR(...) std::tuple<__VA_ARGS__>
#define ELI_DEFAULT_CTOR() std::tuple<>

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
  static const char* ELI_baseName(){ return #Base; }

#define SST_ELI_REGISTER_BASE(Base,...) \
  using Info = ::SST::ELI::FactoryInfoImpl<__VA_ARGS__,void>; \
  SST_ELI_REGISTER_COMMON(Base)

#define SST_ELI_REGISTER_BASE_DEFAULT(Base) \
  using Info = ::SST::ELI::FactoryInfoImpl<void>; \
  SST_ELI_REGISTER_COMMON(Base)

#define SST_ELI_REGISTER_CTORS_COMMON(...) \
  using Ctor = ::SST::ELI::CtorList<__LocalEliName,__VA_ARGS__,void>; \
  static __LocalEliName::Info* getInfo(const std::string& elemlib, const std::string& name){ \
    return Ctor::getInfo(elemlib, name); \
  } \
  template <class... InArgs> static __LocalEliName* create( \
    const std::string& elemlib, const std::string& elem, InArgs&&... args){ \
    return Ctor()(elemlib, elem, std::forward<InArgs>(args)...); \
  }

#define SST_ELI_REGISTER_CTORS(...) \
  SST_ELI_REGISTER_CTORS_COMMON(__VA_ARGS__) \
  template <class __TT, class... __CtorArgs> \
  using DerivedFactory = ::SST::ELI::DerivedFactory<__LocalEliName,__TT,__CtorArgs...>; \
  template <class... __InArgs> static FactoryLibrary<__InArgs...>* getFactoryLibrary(const std::string& name){ \
    return ::SST::ELI::FactoryDatabase::getLibrary<__LocalEliName,__InArgs...>(name); \
  }

//I can make some extra using typedefs because I have only a single ctor
#define SST_ELI_REGISTER_CTOR(...) \
  SST_ELI_REGISTER_CTORS_COMMON(ELI_CTOR(__VA_ARGS__)) \
  template <class __TT> \
  using DerivedFactory = ::SST::ELI::DerivedFactory<__LocalEliName,__TT,__VA_ARGS__>; \
  static FactoryLibrary<__VA_ARGS__>* getFactoryLibrary(const std::string& name){ \
    return SST::ELI::FactoryDatabase::getLibrary<__LocalEliName,__VA_ARGS__>(name); \
  }

//I can make some extra using typedefs because I have only a single ctor
#define SST_ELI_REGISTER_DEFAULT_CTOR() \
  SST_ELI_REGISTER_CTORS_COMMON(ELI_DEFAULT_CTOR()) \
  template <class __TT> \
  using DerivedFactory = ::SST::ELI::DerivedFactory<__LocalEliName,__TT,__VA_ARGS__>; \
  static FactoryLibrary<__VA_ARGS__>* getFactoryLibrary(const std::string& name){ \
    return SST::ELI::FactoryDatabase::getLibrary<__LocalEliName,__VA_ARGS__>(name); \
  }

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

#define SST_ELI_REGISTER_DERIVED(base,cls,lib,name,version,desc) \
  bool ELI_isLoaded() { \
      return SST::ELI::Instantiate<base,cls>::isLoaded(); \
  } \
  SST_ELI_DEFAULT_INFO(lib,name,ELI_FORWARD_AS_ONE(version),desc)

#define SST_ELI_REGISTER_INSTANTIATE_CUSTOM_CTOR(base,cls,lib,name,version,desc,ctor_tuple) \
  bool ELI_isLoaded() { \
      return SST::ELI::InstantiateCustomCtor<base,cls,ctor_tuple>::isLoaded(); \
  } \
  SST_ELI_DEFAULT_INFO(lib,name,ELI_FORWARD_AS_ONE(version),desc)

#define SST_ELI_REGISTER_DERIVED_NEW_CTOR(base,cls,lib,name,version,desc) \
  bool ELI_isLoaded() { \
      return SST::ELI::Instantiate<base,cls>::isLoaded(); \
  } \
  SST_ELI_DEFAULT_INFO(lib,name,ELI_FORWARD_AS_ONE(version),desc)

#define SST_ELI_REGISTER_MODULE(cls,lib,name,version,desc,interface)    \
  SST_ELI_REGISTER_DERIVED(SST::Module,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc) \
  SST_ELI_INTERFACE_INFO(interface)

#define SST_ELI_REGISTER_PARTITIONER(cls,lib,name,version,desc) \
    SST_ELI_REGISTER_DERIVED(SST::Partition::SSTPartitioner,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc)

#define SST_ELI_REGISTER_PYTHON_MODULE(cls,lib,version)    \
  SST_ELI_REGISTER_DERIVED(SST::SSTElementPythonModule,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc)


} //namespace SST

#endif // SST_CORE_ELEMENTINFO_H
