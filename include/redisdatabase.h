#ifndef REDIS_DATABASE_H
#define REDIS_DATABASE_H

#include <string>
#include <mutex>
#include <vector>
#include <chrono>
#include <unordered_map>

class redisdatabase {
public:
    static redisdatabase& getInstance();
    bool flushall();

    // Key/Value Operations
    void set(const std::string& key, const std::string& value);
    bool get(const std::string& key, std::string& value);
    std::vector<std::string> keys();
    std::string type(const std::string& key);
    bool del(const std::string& key);
    bool expire(const std::string& key, int seconds);
    void purgeexpire();
    bool rename(const std::string& oldKey, const std::string& newKey);

    std::vector<std::string> lget(const std::string& key);
    size_t llen(const std::string& key);
    void lpush(const std::string& key, const std::string& value);
    void rpush(const std::string& key, const std::string& value);
    bool lpop(const std::string& key, std::string& value);
    bool rpop(const std::string& key, std::string& value);
    int lrem(const std::string& key, int count, const std::string& value);
    bool lindex(const std::string& key, int index, std::string& value);
    bool lset(const std::string& key, int index, const std::string& value);
    bool hset(const std::string& key, const std::string& field, const std::string& value);
    bool hget(const std::string& key, const std::string& field, std::string& value);
    bool hexists(const std::string& key, const std::string& field);
    bool hdel(const std::string& key, const std::string& field);
    std::unordered_map<std::string, std::string> hgetall(const std::string& key);
    std::vector<std::string> hkeys(const std::string& key);
    std::vector<std::string> hvals(const std::string& key);
    size_t hlen(const std::string& key);
    bool hmset(const std::string& key, const std::vector<std::pair<std::string, std::string>>& fieldValues);

    bool dump(const std::string& filename);
    bool load(const std::string& filename);

private:
    redisdatabase() = default;
    ~redisdatabase() = default;
    redisdatabase(const redisdatabase&) = delete;
    redisdatabase& operator=(const redisdatabase&) = delete;

    std::mutex db_mutex;
    std::unordered_map<std::string, std::string> kv_store;
    std::unordered_map<std::string, std::vector<std::string>> list_store;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hash_store;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> expiry_map;
};

#endif
