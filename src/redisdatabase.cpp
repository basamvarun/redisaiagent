#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include "../include/redisdatabase.h"

redisdatabase& redisdatabase::getInstance() {
    static redisdatabase instance;
    return instance;
}

bool redisdatabase::flushall() {
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store.clear();
    list_store.clear();
    hash_store.clear();
    expiry_map.clear();
    return true;
}

// Key/Value Operations
void redisdatabase::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store[key] = value;
}

bool redisdatabase::get(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    auto it = kv_store.find(key);
    if (it != kv_store.end()) {
        value = it->second;
        return true;
    }
    return false;
}

std::vector<std::string> redisdatabase::keys() {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    std::vector<std::string> result;
    for (const auto& pair : kv_store) {
        result.push_back(pair.first);
    }
    for (const auto& pair : list_store) {
        result.push_back(pair.first);
    }
    for (const auto& pair : hash_store) {
        result.push_back(pair.first);
    }
    return result;
}

std::string redisdatabase::type(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    if (kv_store.find(key) != kv_store.end())
        return "string";
    if (list_store.find(key) != list_store.end())
        return "list";
    if (hash_store.find(key) != hash_store.end())
        return "hash";
    return "none";
}

bool redisdatabase::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    bool erased = false;
    erased |= kv_store.erase(key) > 0;
    erased |= list_store.erase(key) > 0;
    erased |= hash_store.erase(key) > 0;
    expiry_map.erase(key); // Also remove from expiry map
    return erased; // Fixed: was returning false
}

bool redisdatabase::expire(const std::string& key, int seconds) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    bool exists = (kv_store.find(key) != kv_store.end()) ||
        (list_store.find(key) != list_store.end()) ||
        (hash_store.find(key) != hash_store.end());
    if (!exists)
        return false;

    expiry_map[key] = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    return true;
}

void redisdatabase::purgeexpire() {
    auto now = std::chrono::steady_clock::now();
    for (auto it = expiry_map.begin(); it != expiry_map.end(); ) {
        if (now > it->second) {
            // Remove from all stores
            kv_store.erase(it->first);
            list_store.erase(it->first);
            hash_store.erase(it->first);
            it = expiry_map.erase(it);
        }
        else {
            ++it;
        }
    }
}

bool redisdatabase::rename(const std::string& oldKey, const std::string& newKey) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    bool found = false;

    auto itKv = kv_store.find(oldKey);
    if (itKv != kv_store.end()) {
        kv_store[newKey] = itKv->second;
        kv_store.erase(itKv);
        found = true;
    }

    auto itList = list_store.find(oldKey);
    if (itList != list_store.end()) {
        list_store[newKey] = itList->second;
        list_store.erase(itList);
        found = true;
    }

    auto itHash = hash_store.find(oldKey);
    if (itHash != hash_store.end()) {
        hash_store[newKey] = itHash->second;
        hash_store.erase(itHash);
        found = true;
    }

    auto itExpire = expiry_map.find(oldKey);
    if (itExpire != expiry_map.end()) {
        expiry_map[newKey] = itExpire->second;
        expiry_map.erase(itExpire);
    }

    return found;
}
std::vector<std::string> redisdatabase::lget(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    auto it = list_store.find(key);
    if (it != list_store.end()) {
        return it->second;
    }
    return {};
}

size_t redisdatabase::llen(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    auto it = list_store.find(key);
    if (it != list_store.end())
        return it->second.size();
    return 0;
}

void redisdatabase::lpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    list_store[key].insert(list_store[key].begin(), value);
}

void redisdatabase::rpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    list_store[key].push_back(value);
}

bool redisdatabase::lpop(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    auto it = list_store.find(key);
    if (it != list_store.end() && !it->second.empty()) {
        value = it->second.front();
        it->second.erase(it->second.begin());
        return true;
    }
    return false;
}

bool redisdatabase::rpop(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    auto it = list_store.find(key);
    if (it != list_store.end() && !it->second.empty()) {
        value = it->second.back();
        it->second.pop_back();
        return true;
    }
    return false;
}

int redisdatabase::lrem(const std::string& key, int count, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    int removed = 0;
    auto it = list_store.find(key);
    if (it == list_store.end())
        return 0;

    auto& lst = it->second;

    if (count == 0) {
        // Remove all occurrences
        auto new_end = std::remove(lst.begin(), lst.end(), value);
        removed = std::distance(new_end, lst.end());
        lst.erase(new_end, lst.end());
    }
    else if (count > 0) {
        // Remove from head to tail
        for (auto iter = lst.begin(); iter != lst.end() && removed < count; ) {
            if (*iter == value) {
                iter = lst.erase(iter);
                ++removed;
            }
            else {
                ++iter;
            }
        }
    }
    else {
        // Remove from tail to head (count is negative)
        count = -count; // Make it positive for comparison
        for (auto riter = lst.rbegin(); riter != lst.rend() && removed < count; ) {
            if (*riter == value) {
                // Convert reverse iterator to forward iterator
                auto fwdIter = std::next(riter).base();
                fwdIter = lst.erase(fwdIter);
                ++removed;
                // Update reverse iterator after erase
                riter = std::reverse_iterator<std::vector<std::string>::iterator>(fwdIter);
            }
            else {
                ++riter;
            }
        }
    }
    return removed;
}

bool redisdatabase::lindex(const std::string& key, int index, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    auto it = list_store.find(key);
    if (it == list_store.end())
        return false;

    const auto& lst = it->second;
    if (index < 0)
        index = static_cast<int>(lst.size()) + index;
    if (index < 0 || index >= static_cast<int>(lst.size()))
        return false;

    value = lst[index];
    return true;
}

bool redisdatabase::lset(const std::string& key, int index, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    auto it = list_store.find(key);
    if (it == list_store.end())
        return false;

    auto& lst = it->second;
    if (index < 0)
        index = static_cast<int>(lst.size()) + index;
    if (index < 0 || index >= static_cast<int>(lst.size()))
        return false;

    lst[index] = value;
    return true;
}

// Hash Operations
bool redisdatabase::hset(const std::string& key, const std::string& field, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    hash_store[key][field] = value;
    return true;
}

bool redisdatabase::hget(const std::string& key, const std::string& field, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    auto it = hash_store.find(key);
    if (it != hash_store.end()) {
        auto f = it->second.find(field);
        if (f != it->second.end()) {
            value = f->second;
            return true;
        }
    }
    return false;
}

bool redisdatabase::hexists(const std::string& key, const std::string& field) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    auto it = hash_store.find(key);
    if (it != hash_store.end())
        return it->second.find(field) != it->second.end();
    return false;
}

bool redisdatabase::hdel(const std::string& key, const std::string& field) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    auto it = hash_store.find(key);
    if (it != hash_store.end())
        return it->second.erase(field) > 0;
    return false;
}

std::unordered_map<std::string, std::string> redisdatabase::hgetall(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    auto it = hash_store.find(key);
    if (it != hash_store.end())
        return it->second;
    return {};
}

std::vector<std::string> redisdatabase::hkeys(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    std::vector<std::string> fields;
    auto it = hash_store.find(key);
    if (it != hash_store.end()) {
        for (const auto& pair : it->second)
            fields.push_back(pair.first);
    }
    return fields;
}

std::vector<std::string> redisdatabase::hvals(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    std::vector<std::string> values;
    auto it = hash_store.find(key);
    if (it != hash_store.end()) {
        for (const auto& pair : it->second)
            values.push_back(pair.second);
    }
    return values;
}

size_t redisdatabase::hlen(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    auto it = hash_store.find(key);
    return (it != hash_store.end()) ? it->second.size() : 0;
}

bool redisdatabase::hmset(const std::string& key, const std::vector<std::pair<std::string, std::string>>& fieldValues) {
    std::lock_guard<std::mutex> lock(db_mutex);
    for (const auto& pair : fieldValues) {
        hash_store[key][pair.first] = pair.second;
    }
    return true;
}

bool redisdatabase::dump(const std::string& filename) {
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeexpire();
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) return false;

    // Save key-value pairs
    for (const auto& kv : kv_store) {
        ofs << "K " << kv.first << " " << kv.second << "\n";
    }

    // Save lists
    for (const auto& kv : list_store) {
        ofs << "L " << kv.first;
        for (const auto& item : kv.second)
            ofs << " " << item;
        ofs << "\n";
    }

    // Save hashes
    for (const auto& kv : hash_store) {
        ofs << "H " << kv.first;
        for (const auto& field_val : kv.second)
            ofs << " " << field_val.first << ":" << field_val.second;
        ofs << "\n";
    }

    return true;
}

bool redisdatabase::load(const std::string& filename) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) return false;

    // Clear existing data
    kv_store.clear();
    list_store.clear();
    hash_store.clear();
    expiry_map.clear();

    std::string line;
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        char type;
        iss >> type;

        if (type == 'K') {
            std::string key, value;
            iss >> key >> value;
            kv_store[key] = value;
        }
        else if (type == 'L') {
            std::string key;
            iss >> key;
            std::string item;
            std::vector<std::string> list;
            while (iss >> item)
                list.push_back(item);
            list_store[key] = list;
        }
        else if (type == 'H') {
            std::string key;
            iss >> key;
            std::unordered_map<std::string, std::string> hash;
            std::string pair;
            while (iss >> pair) {
                auto pos = pair.find(':');
                if (pos != std::string::npos) {
                    std::string field = pair.substr(0, pos);
                    std::string value = pair.substr(pos + 1);
                    hash[field] = value;
                }
            }
            hash_store[key] = hash;
        }
    }
    return true;
}
