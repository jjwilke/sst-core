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


#include "sst_config.h"
#include <sst/core/warnmacros.h>
#include "sst/core/factory.h"

#include <set>
#include <tuple>
#include <stdio.h>

#include "sst/core/part/sstpart.h"
#include "sst/core/simulation.h"
#include "sst/core/component.h"
#include "sst/core/subcomponent.h"
#include "sst/core/element.h"
#include <sst/core/elementinfo.h>
#include "sst/core/params.h"
#include "sst/core/linkMap.h"

// Statistic Output Objects
#include <sst/core/statapi/statoutputconsole.h>
#include <sst/core/statapi/statoutputtxt.h>
#include <sst/core/statapi/statoutputcsv.h>
#include <sst/core/statapi/statoutputjson.h>
#ifdef HAVE_HDF5
#include <sst/core/statapi/statoutputhdf5.h>
#endif
#include <sst/core/statapi/statbase.h>
#include <sst/core/statapi/stataccumulator.h>
#include <sst/core/statapi/stathistogram.h>
#include <sst/core/statapi/statnull.h>
#include <sst/core/statapi/statuniquecount.h>

using namespace SST::Statistics;

namespace SST {

Factory* Factory::instance = NULL;
//SSTElementPythonModuleOldELI* PythonModuleDocOldELI::instance = NULL;


ElementLibraryInfo empty_eli = {
    "empty-eli",
    "ELI that gets returned when new ELI is used",
    NULL,
    NULL,   // Events
    NULL,   // Introspectors
    NULL,
    NULL,
    NULL, // partitioners,
    NULL,  // Python Module Generator
    NULL // generators,
};


Factory::Factory(std::string searchPaths) :
    searchPaths(searchPaths),
    out(Output::getDefaultObject())
{
    if ( instance ) out.fatal(CALL_INFO, -1, "Already initialized a factory.\n");
    instance = this;
    loader = new ElemLoader(searchPaths);
}


Factory::~Factory()
{
    delete loader;
}



static bool checkPort(const std::string &def, const std::string &offered)
{
    const char * x = def.c_str();
    const char * y = offered.c_str();

    /* Special case.  Name of '*' matches everything */
    if ( *x == '*' && *(x+1) == '\0' ) return true;

    do {
        if ( *x == '%' && (*(x+1) == '(' || *(x+1) == 'd') ) {
            // We have a %d or %(var)d to eat
            x++;
            if ( *x == '(' ) {
                while ( *x && (*x != ')') ) x++;
                x++;  /* *x should now == 'd' */
            }
            if ( *x != 'd') /* Malformed string.  Fail all the things */
                return false;
            x++; /* Finish eating the variable */
            /* Now, eat the corresponding digits of y */
            while ( *y && isdigit(*y) ) y++;
        }
        if ( *x != *y ) return false;
        if ( *x == '\0' ) return true;
        x++;
        y++;
    } while ( *x && *y );
    if ( *x != *y ) return false; // aka, both NULL
    return true;
}

bool Factory::isPortNameValid(const std::string &type, const std::string port_name)
{
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(type);
    // ensure library is already loaded...
    if (loaded_libraries.find(elemlib) == loaded_libraries.end()) {
        findLibrary(elemlib);
    }


    const std::vector<std::string> *portNames = NULL;

    auto* compInfo = Component::getInfo(elemlib,elem);
    if (compInfo) {
      portNames = &compInfo->getPortnames();
    } else {
      auto* subcompInfo = SubComponent::getInfo(elemlib,elem);
      if (subcompInfo) portNames = &subcompInfo->getPortnames();
    }

    std::string tmp = elemlib + "." + elem;

    if ( portNames == NULL ) {
        out.fatal(CALL_INFO, -1,"can't find requested component or subcomponent %s.\n ", tmp.c_str());
        return false;
    }

    for ( auto p : *portNames ) {
        if ( checkPort(p, port_name) )
            return true;
    }
    return false;
}


Component*
Factory::CreateComponent(ComponentId_t id, 
                         const std::string &type,
                         Params& params)
{
    std::string elemlib, elem;
    
    std::tie(elemlib, elem) = parseLoadName(type);

    // ensure library is already loaded...
    requireLibrary(elemlib);

    std::lock_guard<std::recursive_mutex> lock(factoryMutex);
    auto* compInfo = Component::getInfo(elemlib,elem);
    if (compInfo) {
      LinkMap *lm = Simulation::getSimulation()->getComponentLinkMap(id);
      lm->setAllowedPorts(&(compInfo->getPortnames()));

      loadingComponentType = type;
      params.pushAllowedKeys(compInfo->getParamNames());
      Component *ret = Component::create(elemlib,elem,id,params);
      params.popAllowedKeys();
      loadingComponentType = "";
      return ret;
    }

    // If we make it to here, component not found
    out.fatal(CALL_INFO, -1,"can't find requested component %s.\n ", type.c_str());
    return NULL;
}

bool
Factory::DoesSubComponentSlotExist(const std::string& type, const std::string& slotName)
{
    std::string compTypeToLoad = type;
    
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(compTypeToLoad);

    // ensure library is already loaded...
    requireLibrary(elemlib);

    std::lock_guard<std::recursive_mutex> lock(factoryMutex);

    auto* compLib = ELI::InfoDatabase::getLibrary<Component>(elemlib);
    if (compLib){
      auto* info = compLib->getInfo(elem);
      if (info){
        for (auto& item : info->getSubComponentSlots()){
          if (item.name == slotName) return true;
        }
      }
      return false;
    }

    auto* subLib = ELI::InfoDatabase::getLibrary<SubComponent>(elemlib);
    if (subLib){
      auto* info = subLib->getInfo(elem);
      if (info){
        for (auto& item : info->getSubComponentSlots()){
          if (item.name == slotName) return true;
        }
      }
      return false;
    }

    // If we get to here, element doesn't exist
    out.fatal(CALL_INFO, -1,"can't find requested component/subcomponent %s\n ", type.c_str());
    return false;
}

bool 
Factory::DoesComponentInfoStatisticNameExist(const std::string& type, const std::string& statisticName)
{
    std::string compTypeToLoad = type;
    if (true == type.empty()) { 
        compTypeToLoad = loadingComponentType;
    }
    
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(compTypeToLoad);

    // ensure library is already loaded...
    requireLibrary(elemlib);

    std::lock_guard<std::recursive_mutex> lock(factoryMutex);

    auto* lib = ELI::InfoDatabase::getLibrary<Component>(elemlib);
    if (lib){
      auto* info = lib->getInfo(elem);
      if (info){
        for (auto& item : info->getStatnames()) {
            if (statisticName == item) {
                return true;
            }
        }
        return false;
      }
    }
    
    // If we get to here, element doesn't exist
    out.fatal(CALL_INFO, -1,"can't find requested component %s.\n ", type.c_str());
    return false;
}

bool 
Factory::DoesSubComponentInfoStatisticNameExist(const std::string& type, const std::string& statisticName)
{
    std::string compTypeToLoad = type;
    if (true == type.empty()) { 
        compTypeToLoad = loadingComponentType;
    }
    
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(compTypeToLoad);

    // ensure library is already loaded...
    requireLibrary(elemlib);

    std::lock_guard<std::recursive_mutex> lock(factoryMutex);

    auto* lib = ELI::InfoDatabase::getLibrary<SubComponent>(elemlib);
    if (lib){
      auto* info = lib->getInfo(elem);
      if (info){
        for ( auto& item : info->getStatnames() ) {
            if ( statisticName == item ) {
                return true;
            }
        }
        return false;
      }
    }

    // If we get to here, element doesn't exist
    out.fatal(CALL_INFO, -1,"can't find requested subcomponent %s.\n ", type.c_str());
    return false;
}

uint8_t 
Factory::GetComponentInfoStatisticEnableLevel(const std::string& type, const std::string& statisticName)
{
    std::string compTypeToLoad = type;
    if (true == type.empty()) { 
        compTypeToLoad = loadingComponentType;
    }
    
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(compTypeToLoad);

    // ensure library is already loaded...
    requireLibrary(elemlib);

    std::lock_guard<std::recursive_mutex> lock(factoryMutex);

    auto* compLib = ELI::InfoDatabase::getLibrary<Component>(elemlib);
    if (compLib){
      auto* info = compLib->getInfo(elem);
      if (info){
        for ( auto& item : info->getValidStats() ) {
            if ( statisticName == item.name ) {
                return item.enableLevel;
            }
        }
      }
      return 0;
    }

    auto* subLib = ELI::InfoDatabase::getLibrary<SubComponent>(elemlib);
    if (subLib){
      auto* info = subLib->getInfo(elem);
      if (info){
        for ( auto& item : info->getValidStats() ) {
            if ( statisticName == item.name ) {
                return item.enableLevel;
            }
        }
      }
      return 0;
    }

    // If we get to here, element doesn't exist
    out.fatal(CALL_INFO, -1,"can't find requested component %s.\n ", type.c_str());
    return 0;
}

std::string 
Factory::GetComponentInfoStatisticUnits(const std::string& type, const std::string& statisticName)
{
    std::string compTypeToLoad = type;
    if (true == type.empty()) { 
        compTypeToLoad = loadingComponentType;
    }
    
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(compTypeToLoad);

    // ensure library is already loaded...
    if (loaded_libraries.find(elemlib) == loaded_libraries.end()) {
        findLibrary(elemlib);
    }

    auto* compLib = ELI::InfoDatabase::getLibrary<Component>(elemlib);
    if (compLib){
      auto* info = compLib->getInfo(elem);
      if (info){
        for ( auto& item : info->getValidStats() ) {
            if ( statisticName == item.name ) {
                return item.units;
            }
        }
      }
      return 0;
    }

    auto* subLib = ELI::InfoDatabase::getLibrary<SubComponent>(elemlib);
    if (subLib){
      auto* info = subLib->getInfo(elem);
      if (info){
        for ( auto& item : info->getValidStats() ) {
            if ( statisticName == item.name ) {
                return item.units;
            }
        }
      }
      return 0;
    }

    // If we get to here, element doesn't exist
    out.fatal(CALL_INFO, -1,"can't find requested component %s.\n ", type.c_str());
    return 0;
}

Module*
Factory::CreateModule(const std::string& type, Params& params)
{
    if("" == type) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO,
                -1, "Error: Core attempted to load an empty module name, did you miss a module string in your input deck?\n");
    }

    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(type);

    requireLibrary(elemlib);
    std::lock_guard<std::recursive_mutex> lock(factoryMutex);

    auto* lib = ELI::InfoDatabase::getLibrary<Module>(elemlib);
    if (lib){
      auto* info = lib->getInfo(elem);
      if (info){
        params.pushAllowedKeys(info->getParamNames());
        Module *ret = Module::create(elemlib,elem,params);
        params.popAllowedKeys();
        return ret;
      }
    }
    
    // If we get to here, element doesn't exist
    out.fatal(CALL_INFO, -1, "can't find requested module %s.\n ", type.c_str());
    return NULL;
}


Module* 
Factory::LoadCoreModule_StatisticOutputs(const std::string& type, Params& params)
{
  return Create<StatisticOutput>(type, params, params);
}

Module*
Factory::CreateModuleWithComponent(const std::string& type, Component* comp, Params& params)
{
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(type);

    // ensure library is already loaded...
    requireLibrary(elemlib);

    std::lock_guard<std::recursive_mutex> lock(factoryMutex);

    auto* lib = ELI::InfoDatabase::getLibrary<Module>(elemlib);
    if (lib){
      auto* info = lib->getInfo(elem);
      if (info){
        params.pushAllowedKeys(info->getParamNames());
        Module *ret = Module::create(elemlib,elem,comp,params);
        params.popAllowedKeys();
        return ret;
      }
    }

    // If we get to here, element doesn't exist
    out.fatal(CALL_INFO, -1,"can't find requested module %s.\n ", type.c_str());
    return NULL;
}



SubComponent*
Factory::CreateSubComponent(const std::string& type, Component* comp, Params& params)
{
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(type);

    // ensure library is already loaded...
    requireLibrary(elemlib);

    std::lock_guard<std::recursive_mutex> lock(factoryMutex);
    // Check to see if library is loaded into new
    // ElementLibraryDatabase

    auto* lib = ELI::InfoDatabase::getLibrary<SubComponent>(elemlib);
    if (lib){
      auto* info = lib->getInfo(elem);
      if (info){
        params.pushAllowedKeys(info->getParamNames());
        SubComponent* ret = SubComponent::create(elemlib, elem, comp, params);
        params.popAllowedKeys();
        return ret;
      }
    }

    // If we get to here, element doesn't exist
    out.fatal(CALL_INFO, -1,"can't find requested subcomponent %s.\n ", type.c_str());
    return NULL;
}


void
Factory::RequireEvent(const std::string& eventname)
{
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(eventname);

    // ensure library is already loaded...
    requireLibrary(elemlib);

    // All we really need to do is make sure the library is loaded.
    // We no longer have events in the ELI

    // std::lock_guard<std::recursive_mutex> lock(factoryMutex);

    // // initializer fires at library load time, so all we have to do is
    // // make sure the event actually exists...
    // if (found_events.find(eventname) == found_events.end()) {
    //     out.fatal(CALL_INFO, -1,"can't find event %s in %s\n ", eventname.c_str(),
    //            searchPaths.c_str() );
    // }
}

Partition::SSTPartitioner*
Factory::CreatePartitioner(std::string name, RankInfo total_ranks, RankInfo my_rank, int verbosity)
{
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(name);

    // ensure library is already loaded...
    requireLibrary(elemlib);

    auto* lib = ELI::InfoDatabase::getLibrary<Partition::SSTPartitioner>(elemlib);
    if (lib){
      auto* info = lib->getInfo(elem);
      if (info){
        return Partition::SSTPartitioner::create(elemlib, elem, total_ranks, my_rank, verbosity);
      }
    }

    // If we get to here, element doesn't exist
    out.fatal(CALL_INFO, -1,"Error: Unable to find requested partitioner %s, check --help for information on partitioners.\n ", name.c_str());
    return NULL;
}

generateFunction
Factory::GetGenerator(std::string name)
{
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(name);

    // ensure library is already loaded...
    requireLibrary(elemlib);

    // Look for the generator
    std::string tmp = elemlib + "." + elem;

    std::lock_guard<std::recursive_mutex> lock(factoryMutex);
    eig_map_t::iterator eii = found_generators.find(tmp);
    if ( eii == found_generators.end() ) {
        out.fatal(CALL_INFO, -1,"can't find requested generator %s.\n ", tmp.c_str());
        return NULL;
    }

    const ElementInfoGenerator *ei = eii->second;
    return ei->func;
}


// genPythonModuleFunction
SSTElementPythonModule*
Factory::getPythonModule(std::string name)
{
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(name);

    auto* lib = ELI::InfoDatabase::getLibrary<SSTElementPythonModule>(elemlib);
    if (lib) {
      auto* info = lib->getInfo(elem);
      if (info){
        //this looks funny with name passed 3 times
        //first is library
        //derived name is always also the library
        //and finally the libname needs to be passed as an arg to ctor
        return SSTElementPythonModule::create(elemlib,elemlib,elemlib);
      }
    }

    return NULL;
}



bool Factory::hasLibrary(std::string elemlib)
{
    return (NULL != findLibrary(elemlib, true));
}


void Factory::requireLibrary(std::string &elemlib)
{
    if ( elemlib == "sst" ) return;
    (void)findLibrary(elemlib, true);
}


void Factory::getLoadedLibraryNames(std::set<std::string>& lib_names)
{
    for ( eli_map_t::const_iterator i = loaded_libraries.begin();
          i != loaded_libraries.end(); ++i)
        {
            lib_names.insert(i->first);
        }
}

void Factory::loadUnloadedLibraries(const std::set<std::string>& lib_names)
{
    for ( std::set<std::string>::const_iterator i = lib_names.begin();
          i != lib_names.end(); ++i )
        {
            findLibrary(*i);
        }
}

    
const ElementLibraryInfo*
Factory::findLibrary(std::string elemlib, bool showErrors)
{
    if ( elemlib == "sst" ) return nullptr;
    const ElementLibraryInfo *eli = nullptr;
    std::lock_guard<std::recursive_mutex> lock(factoryMutex);

    eli_map_t::iterator elii = loaded_libraries.find(elemlib);
    if (elii != loaded_libraries.end()) return elii->second;


    // eli = loader->loadLibrary(elemlib, showErrors);
    eli = loadLibrary(elemlib, showErrors);
    if (nullptr == eli) return nullptr;



    // Convert the old ELI data structures into the new ELI data
    // structures
    loaded_libraries[elemlib] = eli;

    if (NULL != eli->components) { //use NULL I guess for C?
      const auto *eic = eli->components;
      auto* infolib = Component::getInfoLibrary(elemlib);
      auto* factlib = Component::getFactoryLibrary(elemlib);
      while (NULL != eic->name) {
        factlib->addFactory(eic->name, new Component::DerivedFactory<OldELITag>(eic->alloc));
        infolib->addInfo(new Component::DerivedInfo<OldELITag>(elemlib,eic));
      }
    }

    if (NULL != eli->subcomponents) { //use NULL I guess for C?
      const auto *eis = eli->subcomponents;
      auto* infolib = SubComponent::getInfoLibrary(elemlib);
      auto* factlib = SubComponent::getFactoryLibrary(elemlib);
      while (NULL != eis->name) {
        factlib->addFactory(eis->name, new SubComponent::DerivedFactory<OldELITag>(eis->alloc));
        infolib->addInfo(new SubComponent::DerivedInfo<OldELITag>(elemlib, eis));
        eis++;
      }
    }

    if (NULL != eli->partitioners) { //use NULL I guess for C?
      const auto *eis = eli->partitioners;
      auto* infolib = Partition::SSTPartitioner::getInfoLibrary(elemlib);
      auto* factlib = Partition::SSTPartitioner::getFactoryLibrary(elemlib);
      while (NULL != eis->name) {
        factlib->addFactory(eis->name, new Partition::SSTPartitioner::DerivedFactory<OldELITag>(eis->func));
        infolib->addInfo(new Partition::SSTPartitioner::DerivedInfo<OldELITag>(elemlib, eis));
        eis++;
      }
    }


    if (NULL != eli->modules) {
      const ElementInfoModule *eim = eli->modules;
      auto* infolib = Module::getInfoLibrary(elemlib);
      auto* withlib = Module::getFactoryLibrary<Component*,SST::Params&>(elemlib);
      auto* wolib = Module::getFactoryLibrary<SST::Params&>(elemlib);
      while (NULL != eim->name) {
        auto* info = new Module::DerivedInfo<OldELITag>(elemlib, eim);
        infolib->addInfo(info);

        auto* withComp = new Module::DerivedFactory<OldELITag,Component*,SST::Params&>(eim->alloc_with_comp);
        withlib->addFactory(eim->name, withComp);

        auto* withoutComp = new Module::DerivedFactory<OldELITag,SST::Params&>(eim->alloc);
        wolib->addFactory(eim->name, withoutComp);
        eim++;
      }
    }


    if (NULL != eli->generators) {
        const ElementInfoGenerator *eig = eli->generators;
        while (NULL != eig->name) {
            std::string tmp = elemlib + "." + eig->name;
            found_generators[tmp] = eig;
            eig++;
        }
    }

    if (NULL != eli->pythonModuleGenerator ) {
      auto* factLib = SSTElementPythonModule::getFactoryLibrary(elemlib);
      auto* fact = new SSTElementPythonModule::DerivedFactory<SSTElementPythonModuleOldELI>(eli->pythonModuleGenerator);
      factLib->addFactory(elemlib, fact);

      auto* infoLib = SSTElementPythonModule::getInfoLibrary(elemlib);
      auto* info = new SSTElementPythonModule::DerivedInfo<OldELITag>(elemlib, eli);
      infoLib->addInfo(info);
    }
    
    return eli;
}



std::pair<std::string, std::string>
Factory::parseLoadName(const std::string& wholename)
{
    std::size_t found = wholename.find_first_of(".");
    if (found == std::string::npos) {
        return make_pair(wholename, wholename);
    } else {
        std::string eli(wholename, 0, found);
        std::string el(wholename, (size_t)(found + 1));
        return make_pair(eli, el);
    }
}

const ElementLibraryInfo* Factory::loadLibrary(std::string name, bool showErrors)
{
    if ( name == "sst" ) return NULL;
    const ElementLibraryInfo* eli = loader->loadLibrary(name, showErrors);

    if ( NULL == eli ) {
        // Check to see if this library loaded into the new ELI
        // Database
        if (ELI::DataBase::isLoaded(name)) {
            // Need to just return the empty ElementLibraryInfo
            return &empty_eli;
        }
    }
    
    if (NULL == eli) {
        if (showErrors) {
            fprintf(stderr, "Could not find ELI block %s_eli in %s\n",
                    name.c_str(), name.c_str());
        }
    }
    return eli;
}

} //namespace SST
