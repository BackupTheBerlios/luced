/////////////////////////////////////////////////////////////////////////////////////
//
//   LucED - The Lucid Editor
//
//   Copyright (C) 2005-2006 Oliver Schmidt, osch@luced.de
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License Version 2 as published
//   by the Free Software Foundation in June 1991.
//
//   This program is distributed in the hope that it will be useful, but WITHOUT
//   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//   more details.
//
//   You should have received a copy of the GNU General Public License along with 
//   this program; if not, write to the Free Software Foundation, Inc., 
//   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
/////////////////////////////////////////////////////////////////////////////////////

#ifndef HASHMAP_H
#define HASHMAP_H

#include <ext/hash_map>
#include <string>

namespace LucED {

#ifdef HASH_MAP_UNDER_STD
using std::hash_map;
#else
using __gnu_cxx::hash_map;
#endif
using std::string;

template<class T> class HashFunction;

template<> class HashFunction<string>
{
public:
    size_t operator()(const string& key) const {
        size_t rslt = 0;
        for (int i = 0; i < key.length(); ++i) {
            rslt = 5 * rslt + key[i];
        }
        return rslt;
    }
};

template<> class HashFunction<long>
{
public:
    size_t operator()(long key) const {
        return (size_t) key;
    }
};

template<> class HashFunction<unsigned long>
{
public:
    size_t operator()(unsigned long key) const {
        return (size_t) key;
    }
};

template<class V> class HashMapValue
{
public:
    HashMapValue() : valid(false) {}
    HashMapValue(const V& v) : valid(true), v(v) {}
    bool isValid() const { return valid; }
    operator V()   const { return v; }
    V get()   const { return v; }
private:
    bool valid;
    V v;
};

template<class K, class V, class H = HashFunction<K> > class HashMap
{
public:
    typedef HashMapValue<V> Value;
    
    void set(const K& key, const V& value) {
        map[key] = value;
    }
    Value get(const K& key) const {
        typename hash_map<K,V,H>::const_iterator foundPair = map.find(key);
        if (foundPair == map.end()) {
            return Value();
        } else {
            return Value(foundPair->second);
        }
    }
    bool hasKey(const K& key) const {
        typename hash_map<K,V,H>::const_iterator foundPair = map.find(key);
        return foundPair != map.end();
    }
    void remove(const K& key) {
        map.erase(key);
    }
private:
    hash_map<K,V,H> map;
};


} // namespace LucED

#endif // HASHMAP_H
