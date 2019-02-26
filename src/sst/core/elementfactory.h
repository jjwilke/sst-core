
#ifndef SST_CORE_FACTORY_INFO_H
#define SST_CORE_FACTORY_INFO_H

#include <oldELI.h>
#include <type_traits>

namespace SST {
namespace ELI {

template <class Base, class... Args>
struct Factory
{
  typedef Base* (*createFxn)(Args...);

  virtual Base* create(Args... ctorArgs) = 0;
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


template <class Base, class T>
struct InstantiateFactory {
  static bool isLoaded() {
    return loaded;
  }

  static const bool loaded;
};

template <class Base, class T>
 const bool InstantiateFactory<Base,T>::loaded = Base::Ctor::template add<0,T>();

template <class Base, class T, class Enable=void>
struct Allocator
{
  template <class... Args>
  T* operator()(Args&&... args){
    return new T(std::forward<Args>(args)...);
  }
};

template <class Base, class T>
struct CachedAllocator
{
  template <class... Args>
  Base* operator()(Args&&... ctorArgs) {
    if (!cached_){
      cached_ = new T(std::forward<Args>(ctorArgs)...);
    }
    return cached_;
  }

  static Base* cached_;
};
template <class Base, class T>
  Base* CachedAllocator<Base,T>::cached_ = nullptr;

template <class Base, class T, class... Args>
struct DerivedFactory : public Factory<Base,Args...>
{
  Base* create(Args... ctorArgs) override {
    return Allocator<Base,T>()(std::forward<Args>(ctorArgs)...);
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

template <class T, class U>
struct is_tuple_constructible : public std::false_type {};

template <class T, class... Args>
struct is_tuple_constructible<T, std::tuple<Args...>> :
  public std::is_constructible<T, Args...>
{
};

struct FactoryDatabase {
  template <class T, class... Args>
  static FactoryLibrary<T,Args...>* getLibrary(const std::string& name){
    return FactoryLibraryDatabase<T,Args...>::getLibrary(name);
  }
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

template <class Base, class Ctor, class... Ctors>
struct CtorList : public CtorList<Base,Ctors...>
{
  template <int NumValid, class T, class U=T>
  static typename std::enable_if<std::is_abstract<U>::value || is_tuple_constructible<U,Ctor>::value, bool>::type
  add(){
    //if abstract, force an allocation to generate meaningful errors
    auto* fact = ElementsFactory<Base,Ctor>::template makeFactory<U>();
    ElementsFactory<Base,Ctor>::getLibrary(T::ELI_getLibrary())->addFactory(T::ELI_getName(), fact);
    return CtorList<Base,Ctors...>::template add<NumValid+1,T>();
  }

  template <int NumValid, class T, class U=T>
  static typename std::enable_if<!std::is_abstract<U>::value && !is_tuple_constructible<U,Ctor>::value, bool>::type
  add(){
    return CtorList<Base,Ctors...>::template add<NumValid,T>();
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

template <int NumValid>
struct NoValidConstructorsForDerivedType {
  static constexpr bool atLeastOneValidCtor = true;
};

template <> class NoValidConstructorsForDerivedType<0> {};

template <class Base> struct CtorList<Base,void>
{
  template <int numValidCtors, class T>
  static bool add(){
    return NoValidConstructorsForDerivedType<numValidCtors>::atLeastOneValidCtor;
  }
};

}
}

#define ELI_CTOR(...) std::tuple<__VA_ARGS__>
#define ELI_DEFAULT_CTOR() std::tuple<>


#define SST_ELI_REGISTER_CTORS_COMMON(...) \
  using Ctor = ::SST::ELI::CtorList<__LocalEliName,__VA_ARGS__,void>; \
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
  using DerivedFactory = ::SST::ELI::DerivedFactory<__LocalEliName,__TT>; \
  static FactoryLibrary<>* getFactoryLibrary(const std::string& name){ \
    return SST::ELI::FactoryDatabase::getLibrary<__LocalEliName>(name); \
  }

#endif

