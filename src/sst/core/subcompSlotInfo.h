#ifndef SST_CORE_SUBCOMPSLOTINFO_H
#define SST_CORE_SUBCOMPSLOTINFO_H

#include <string>
#include <vector>

#include <sst/core/elibase.h>
#include <sst/core/oldELI.h>

namespace SST {
namespace ELI {

template <class T, class Enable=void>
struct InfoSubs {
  static const std::vector<SST::ElementInfoSubComponentSlot>& get() {
    static std::vector<SST::ElementInfoSubComponentSlot> var = { };
    return var;
  }
};

template <class T>
struct InfoSubs<T,
      typename MethodDetect<decltype(T::ELI_getSubComponentSlots())>::type> {
  static const std::vector<SST::ElementInfoSubComponentSlot>& get() {
    return T::ELI_getSubComponentSlots();
  }
};

class ProvidesSubComponentSlots {
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
  template <class T> ProvidesSubComponentSlots(T* UNUSED(t)) :
    slots_(InfoSubs<T>::get())
  {
  }

  template <class U> ProvidesSubComponentSlots(OldELITag& UNUSED(tag), U* u)
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

}
}

#define SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(...)                              \
    static const std::vector<SST::ElementInfoSubComponentSlot>& ELI_getSubComponentSlots() { \
    static std::vector<SST::ElementInfoSubComponentSlot> var = { __VA_ARGS__ } ; \
        return var; \
    }

#endif

