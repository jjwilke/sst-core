#ifndef SST_CORE_INTERFACE_INFO_H
#define SST_CORE_INTERFACE_INFO_H

namespace SST {
namespace ELI {

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

}
}

#define SST_ELI_INTERFACE_INFO(interface) \
  static const std::string ELI_getInterface() {  \
    return interface; \
  }

#endif

