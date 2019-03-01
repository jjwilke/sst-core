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
//

#include <sst_config.h>
#include <sst/core/params.h>

#include <map>
#include <vector>
#include <string>


std::map<std::string, uint32_t> SST::Params::keyMap;
std::vector<std::string> SST::Params::keyMapReverse;
SST::Core::ThreadSafe::Spinlock SST::Params::keyLock;
uint32_t SST::Params::nextKeyID;
bool SST::Params::g_verify_enabled = false;

namespace SST {

bool
Params::getString(const std::string& k, std::string& in) const
{
  verifyParam(k);
  const_iterator i = data.find(getKey(k));
  if (i == data.end()) {
      return false;
  } else {
    in = i->second;
    return true;
  }
}

void
Params::error(const std::string& k, const std::string& val, const std::invalid_argument& e) const
{
  std::string msg = "Params::find(): No conversion for value: key = " + k + ", value =  " + val +
      ".  Original error: " + e.what();
  std::invalid_argument t(msg);
  throw t;
}

/** Print all key/value parameter pairs to specified ostream */
void
Params::print_all_params(std::ostream &os, std::string prefix) const {
    for (const_iterator i = data.begin() ; i != data.end() ; ++i) {
        os << prefix << "key=" << keyMapReverse[i->first] << ", value=" << i->second << std::endl;
    }
}

void
Params::print_all_params(Output &out, std::string prefix) const {
    for (const_iterator i = data.begin() ; i != data.end() ; ++i) {
        out.output("%s%s = %s\n", prefix.c_str(), keyMapReverse[i->first].c_str(), i->second.c_str());
    }
}

/** Add a key value pair into the param object.
 */
void
Params::insert(const std::string& key, const std::string& value, bool overwrite) {
    if ( overwrite ) {
        data[getKey(key)] = value;
    }
    else {
        uint32_t id = getKey(key);
        data.insert(std::make_pair(id, value));
    }
}

void
Params::insert(const Params &params){
    data.insert(params.data.begin(), params.data.end());
}

std::set<std::string>
Params::getKeys() const
{
    std::set<std::string> ret;
    for (const_iterator i = data.begin() ; i != data.end() ; ++i) {
        ret.insert(keyMapReverse[i->first]);
    }
    return ret;
}

Params
Params::find_prefix_params(std::string prefix) const
{
    Params ret;
    ret.enableVerify(false);
    for (const_iterator i = data.begin() ; i != data.end() ; ++i) {
        auto const& paramName = keyMapReverse[i->first];
        std::string key = paramName.substr(0, prefix.length());
        if (key == prefix) {
          auto start = prefix.length();
          if (paramName.at(start) == '.'){
            start = start + 1; //implicitly skip periods
          }
          ret.insert(paramName.substr(start), i->second);
        }
    }
    ret.allowedKeys = allowedKeys;
    ret.enableVerify(verify_enabled);

    return ret;
}

void
Params::serialize_order(SST::Core::Serialization::serializer &ser)
{
    ser & data;
}

uint32_t
Params::getKey(const std::string &str) const
{
    std::lock_guard<SST::Core::ThreadSafe::Spinlock> lock(keyLock);
    std::map<std::string, uint32_t>::iterator i = keyMap.find(str);
    if ( i == keyMap.end() ) {
        return (uint32_t)-1;
    }
    return i->second;
}

void
Params::verifyParam(const key_type &k) const
{
    if ( !g_verify_enabled || !verify_enabled ) return;

    for ( std::vector<KeySet_t>::const_reverse_iterator ri = allowedKeys.rbegin() ; ri != allowedKeys.rend() ; ++ri ) {
        if ( ri->find(k) != ri->end() ) return;
    }

#ifdef USE_PARAM_WARNINGS
    SST::Output outXX("ParamWarning: ", 0, 0, Output::STDERR);
    outXX.output(CALL_INFO, "Warning: Parameter \"%s\" is undocumented.\n", k.c_str());
#endif
}

uint32_t
Params::getKey(const std::string &str)
{
    std::lock_guard<SST::Core::ThreadSafe::Spinlock> lock(keyLock);
    std::map<std::string, uint32_t>::iterator i = keyMap.find(str);
    if ( i == keyMap.end() ) {
        uint32_t id = nextKeyID++;
        keyMap.insert(std::make_pair(str, id));
        keyMapReverse.push_back(str);
        assert(keyMapReverse.size() == nextKeyID);
        return id;
    }
    return i->second;
}

const std::string&
Params::getParamName(uint32_t id)
{
    return keyMapReverse[id];
}


/**
 * @param k   Key to search for
 * @return    True if the params contains the key, false otherwise
 */
bool
Params::contains(const key_type &k) {
    return data.find(getKey(k)) != data.end();
}

/**
 * @param keys   Set of keys to consider valid to add to the stack
 *               of legal keys
 */
void
Params::pushAllowedKeys(const KeySet_t &keys) {
    allowedKeys.push_back(keys);
}

/**
 * Removes the most recent set of keys considered allowed
 */
void
Params::popAllowedKeys() {
    allowedKeys.pop_back();
}

void
Params::find_array(const key_type &k, std::vector<double> &vec) const
{
  return template_find_array<double>(k,vec);
}

void
Params::find_array(const key_type &k, std::vector<uint64_t> &vec) const
{
  return template_find_array<uint64_t>(k,vec);
}

void
Params::find_array(const key_type &k, std::vector<int> &vec) const
{
  return template_find_array<int>(k,vec);
}

void
Params::find_array(const key_type &k, std::vector<std::string> &vec) const
{
  return template_find_array<std::string>(k,vec);
}


}


