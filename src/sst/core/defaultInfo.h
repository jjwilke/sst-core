#ifndef SST_CORE_DEFAULTINFO_H
#define SST_CORE_DEFAULTINFO_H

#include <string>
#include <vector>

namespace SST {
namespace ELI {

class DefaultFactoryInfo {
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
  template <class T> DefaultFactoryInfo(T* UNUSED(t)) :
   lib_(T::ELI_getLibrary()),
   name_(T::ELI_getName()),
   desc_(T::ELI_getDescription()),
   version_(T::ELI_getVersion()),
   file_(T::ELI_getCompileFile()),
   date_(T::ELI_getCompileDate()),
   compiled_(T::ELI_getELICompiledVersion())
  {
  }

  template <class U> DefaultFactoryInfo(OldELITag&& tag, U* u) :
    DefaultFactoryInfo(tag,u)
  {
  }

  template <class U> DefaultFactoryInfo(OldELITag& tag, U* u) :
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


}
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

#define SST_ELI_ELEMENT_VERSION(...) {__VA_ARGS__}

#define ELI_FORWARD_AS_ONE(...) __VA_ARGS__

#endif

