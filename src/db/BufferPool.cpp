#include <db/BufferPool.hpp>
#include <db/Database.hpp>
#include <numeric>
#include <algorithm>
#include <iostream>

using namespace db;

BufferPool::BufferPool() {
   numPages = 0;
}

BufferPool::~BufferPool() {
    std::vector<PageId> toFlush(dirty.begin(), dirty.end());
    for (const auto& pid : toFlush) {
        flushPage(pid);
    } 
    pages.clear();
    dirty.clear();
    lru.clear();
}

Page &BufferPool::getPage(const PageId &pid) {
    updateLRU(pid);

    if (contains(pid)) {
        return this->pages[pid];
    }
    
    if (numPages >= DEFAULT_NUM_PAGES) {
        PageId evict = lru.front(); 

        if (isDirty(evict)) {
            flushPage(evict);
        }

        discardPage(evict);
    }
    Database &db = getDatabase();
    DbFile &file = db.get(pid.file);
    this->pages[pid] = Page();
    file.readPage(this->pages[pid], pid.page);
    numPages++;
    
    return this->pages[pid];
}

void BufferPool::markDirty(const PageId &pid) {
    if (!isDirty(pid)) {
        dirty.insert(pid);
    }
}

bool BufferPool::isDirty(const PageId &pid) const {
    if (!contains(pid)) {
        throw std::runtime_error("Page does not exist in buffer pool.");
    }
    return dirty.find(pid) != dirty.end();
}

bool BufferPool::contains(const PageId &pid) const {
    return pages.find(pid) != pages.end();
}

void BufferPool::discardPage(const PageId &pid) {
    if (!contains(pid)) return;

    pages.erase(pid);
    numPages--;

    dirty.erase(pid);

    auto it = std::find(lru.begin(), lru.end(), pid);
    if (it != lru.end()) {
        lru.erase(it);
    }
}

void BufferPool::flushPage(const PageId &pid) {
    if (contains(pid) && isDirty(pid)) {
        Database &db = getDatabase();
        DbFile &file = db.get(pid.file);

        std::cout << "Flushing page " << pid.page << " to file " << pid.file << std::endl;
        file.writePage(this->pages[pid], pid.page);

        dirty.erase(pid); 
    }
}


void BufferPool::flushFile(const std::string &file) {
    std::vector<PageId> toFlush;
    for (const auto& pid : dirty) {
        if (pid.file == file) {
            toFlush.push_back(pid);
        }
    }
    for (const auto& pid : toFlush) {
        flushPage(pid);
    }
}

void BufferPool::updateLRU(const PageId &pid) {
    if (!lru.empty() && lru.back() == pid) return; 

    auto it = std::find(lru.begin(), lru.end(), pid);
    if (it != lru.end()) {
        lru.erase(it);
    }
    lru.push_back(pid);
}
