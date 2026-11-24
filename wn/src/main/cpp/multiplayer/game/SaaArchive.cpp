#include "SaaArchive.h"

#include "main.h"

#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace {

constexpr uint8_t kMagic[4] = { 0xDA, 0xBC, 0x85, 0x74 };
constexpr int32_t kHeaderSize = 30;

// Метод хранения из заголовка; всё, что не 0, читать прямым смещением нельзя
constexpr uint16_t kStore = 0;

struct SEntry {
    std::string archive;
    uint32_t    offset;
    uint32_t    size;
};

std::vector<std::string> g_archives;
std::unordered_map<std::string, SEntry> g_entries;

// Открывать могут и из потока стриминга, поэтому под замком
std::mutex g_windowLock;
std::unordered_map<void*, uint32_t> g_windows;

uint16_t ReadU16(const uint8_t* p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

uint32_t ReadU32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// Игра просит файлы то через слэш, то через обратный, и в любом регистре
std::string Normalise(const char* path) {
    std::string out;

    for (const char* p = path; p && *p; p++) {
        char c = *p;

        if (c == '\\') {
            c = '/';
        } else if (c >= 'A' && c <= 'Z') {
            c = (char)(c - 'A' + 'a');
        }

        out.push_back(c);
    }

    while (!out.empty() && (out.front() == '/' || out.front() == '.')) {
        out.erase(out.begin());
    }

    return out;
}

} // namespace

bool CSaaArchive::Mount(const char* relativePath) {
    if (!relativePath || !g_pszStorage) {
        return false;
    }

    char full[512];
    snprintf(full, sizeof(full), "%s%s", g_pszStorage, relativePath);

    FILE* f = fopen(full, "rb");
    if (!f) {
        return false;
    }

    fseek(f, 0, SEEK_END);
    const long total = ftell(f);

    g_archives.emplace_back(relativePath);
    const std::string archive = g_archives.back();

    long offset = 0;
    int32_t added = 0;
    uint8_t head[kHeaderSize];

    while (offset + kHeaderSize <= total) {
        if (fseek(f, offset, SEEK_SET) != 0 || fread(head, 1, kHeaderSize, f) != kHeaderSize) {
            break;
        }

        if (memcmp(head, kMagic, sizeof(kMagic)) != 0) {
            Log("SaaArchive: %s — не подпись на %ld, дальше не читаю", relativePath, offset);
            break;
        }

        const uint16_t method = ReadU16(head + 8);
        const uint32_t csize = ReadU32(head + 18);
        const uint16_t namelen = ReadU16(head + 26);
        const uint16_t extralen = ReadU16(head + 28);

        std::string name(namelen, '\0');
        if (namelen && fread(&name[0], 1, namelen, f) != namelen) {
            break;
        }

        const long data = offset + kHeaderSize + namelen + extralen;

        // Папки идут пустыми записями, сжатых в кеше не встречается
        if (namelen && name.back() != '/' && csize && method == kStore) {
            g_entries[Normalise(name.c_str())] = SEntry{ archive, (uint32_t)data, csize };
            added++;
        }

        offset = data + csize;
    }

    fclose(f);

    Log("SaaArchive: %s — записей %d", relativePath, added);
    return added > 0;
}

void CSaaArchive::MountAll() {
    if (!g_pszStorage) {
        return;
    }

    DIR* dir = opendir(g_pszStorage);
    if (!dir) {
        Log("SaaArchive: не открылся корень %s", g_pszStorage);
        return;
    }

    std::vector<std::string> found;

    while (dirent* item = readdir(dir)) {
        const char* name = item->d_name;
        const size_t len = strlen(name);

        if (len > 4 && strcasecmp(name + len - 4, ".saa") == 0) {
            found.emplace_back(name);
        }
    }

    closedir(dir);

    // Порядок имён определяет, какой архив выиграет при совпадении имён внутри
    std::sort(found.begin(), found.end());

    for (const std::string& name : found) {
        Mount(name.c_str());
    }

    if (found.empty()) {
        Log("SaaArchive: в %s нет ни одного .saa — файлы внутри архивов будут недоступны", g_pszStorage);
        return;
    }

    Log("SaaArchive: примонтировано %d, файлов внутри %d", ArchiveCount(), EntryCount());
}

bool CSaaArchive::Find(const char* relativePath, const char*& archive,
                       uint32_t& offset, uint32_t& size) {
    if (!relativePath || g_entries.empty()) {
        return false;
    }

    auto it = g_entries.find(Normalise(relativePath));
    if (it == g_entries.end()) {
        return false;
    }

    archive = it->second.archive.c_str();
    offset = it->second.offset;
    size = it->second.size;

    return true;
}

int32_t CSaaArchive::ArchiveCount() {
    return (int32_t)g_archives.size();
}

int32_t CSaaArchive::EntryCount() {
    return (int32_t)g_entries.size();
}

void CSaaArchive::NoteWindow(void *handle, uint32_t size) {
    if (!handle) {
        return;
    }

    std::lock_guard<std::mutex> guard(g_windowLock);
    g_windows[handle] = size;
}

bool CSaaArchive::WindowSize(void *handle, uint32_t &size) {
    if (!handle) {
        return false;
    }

    std::lock_guard<std::mutex> guard(g_windowLock);

    auto it = g_windows.find(handle);
    if (it == g_windows.end()) {
        return false;
    }

    size = it->second;
    return true;
}

void CSaaArchive::ForgetWindow(void *handle) {
    if (!handle) {
        return;
    }

    std::lock_guard<std::mutex> guard(g_windowLock);
    g_windows.erase(handle);
}
