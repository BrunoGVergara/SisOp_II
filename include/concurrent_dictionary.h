#include <iostream>
#include <mutex>
#include <unordered_map>
#include <shared_mutex>
#include <array>
#include <optional>
#include <vector>

using namespace std;

template <typename K, typename V>
class concurrent_dictionary
{
private:
    unordered_map<K, V> dictionary;
    mutable shared_mutex mutex;

public:
    void insert_or_update(const K &key, const V &value)
    {
        unique_lock lock(mutex);
        dictionary[key] = value;
    }

    bool remove(const K &key)
    {
        unique_lock lock(mutex);
        return dictionary.erase(key) > 0;
    }

    V get(const K &key) const
    {
        shared_lock lock(mutex);
        if (auto it = dictionary.find(key); it != dictionary.end())
        {
            return it->second;
        }
        return V();
    }

    optional<K> findFirstDifferentValue(const V &value, const K &key)
    {
        for (const auto &[k, v] : dictionary)
        {
            if (v == value && k != key)
            {
                return k;
            }
        }
        return std::nullopt;
    }

    bool contains(const K &key) const
    {
        shared_lock lock(mutex);
        return dictionary.count(key) > 0;
    }

    void clear()
    {
        unique_lock lock(mutex);
        dictionary.clear();
    }

    vector<K> keys() const
    {
        shared_lock lock(mutex);
        vector<K> keys;
        keys.reserve(dictionary.size());
        for (const auto &pair : dictionary)
        {
            keys.push_back(pair.first);
        }
        return keys;
    }
};