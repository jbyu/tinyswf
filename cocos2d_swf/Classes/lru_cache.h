#ifndef __LRU__CACHE_H__
#define __LRU__CACHE_H__

#include <vector>
#include <map>

template<class K, class T>
struct LRUCacheEntry {
	K key;
	T data;
	LRUCacheEntry* prev;
	LRUCacheEntry* next;
};

template<class K, class T>
class LRUCache {
private:
	std::map< K, LRUCacheEntry<K,T>* >	_mapping;
	std::vector< LRUCacheEntry<K,T>* >	_freeEntries;
	LRUCacheEntry<K,T> *			head;
	LRUCacheEntry<K,T> *			tail;
	LRUCacheEntry<K,T> *			entries;

public:
	LRUCache(int size) {
        _freeEntries.reserve(size);
		entries = new LRUCacheEntry<K,T>[size];
		for (int i=0; i<size; i++)
			_freeEntries.push_back(entries+i);
		head = new LRUCacheEntry<K,T>;
		tail = new LRUCacheEntry<K,T>;
		head->prev = NULL;
		head->next = tail;
		tail->next = NULL;
		tail->prev = head;
	}

	~LRUCache() {
		delete head;
		delete tail;
		delete [] entries;
	}

	T& insert(K key, T data) {
		LRUCacheEntry<K,T>* node = _mapping[key];
		if (node) {
			// refresh the link list
			detach(node);
			node->data = data;
			attach(node);
		} else {
			if ( _freeEntries.empty() ) {
				node = tail->prev;
				detach(node);
				_mapping.erase(node->key);
				node->key = key;
				node->data = data;
				_mapping[key] = node;
				attach(node);
			} else {
				node = _freeEntries.back();
				_freeEntries.pop_back();
				node->key = key;
				node->data = data;
				_mapping[key] = node;
				attach(node);
			}
		}
        return node->data;
	}

	T* fetch_ptr(K key) {
		LRUCacheEntry<K,T>* node = _mapping[key];
		if (node) {
			detach(node);
			attach(node);
			return &node->data;
		}
		return NULL;
	}

    int size(void) const {
        return _freeEntries.capacity() - _freeEntries.size();
    }

    bool full(void) const {
        return _freeEntries.empty();
    }

    T& getLRU(void) {
        return tail->prev->data;
    }

	std::map< K, LRUCacheEntry<K,T>* > getMaptable() {
        return _mapping;
    }

private:
	void detach(LRUCacheEntry<K,T>* node) {
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}

	void attach(LRUCacheEntry<K,T>* node) {
		node->next = head->next;
		node->prev = head;
		head->next = node;
		node->next->prev = node;
	}
};

#endif//lru_cache