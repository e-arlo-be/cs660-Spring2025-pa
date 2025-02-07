#include <db/Database.hpp>

using namespace db;

BufferPool &Database::getBufferPool() { return bufferPool; }

Database &db::getDatabase() {
    static Database instance;
    return instance;
}

void Database::add(std::unique_ptr<DbFile> file) {
    if (this->catalog.find(file->getName()) != this->catalog.end()) {
        throw std::logic_error("Named file already exists in the catalog");
    }
    else {
        this->catalog[file->getName()] = std::move(file);
    }
}

std::unique_ptr<DbFile> Database::remove(const std::string &name) {
    if (this->catalog.find(name) == this->catalog.end()) {
        throw std::logic_error("No such name in the catalog");
    }
    else {
        this->bufferPool.flushFile(name);
        std::unique_ptr<DbFile> file = std::move(this->catalog[name]);
        this->catalog.erase(name);
        return file;
    }
}

DbFile &Database::get(const std::string &name) const {
    if (this->catalog.find(name) == this->catalog.end()) {
        throw std::logic_error("No such name in the catalog");
    }
    else {
        return *this->catalog.at(name);
    }
}
