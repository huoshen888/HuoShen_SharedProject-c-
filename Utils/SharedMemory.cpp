#include "SharedMemory.h"
#include <algorithm>

bool SharedMemory::Write(const std::string& name, const std::vector<uint8_t>& data) {
    HANDLE hMapping = nullptr;
    void* view = nullptr;

    try {
        DWORD totalSize = static_cast<DWORD>(data.size() + sizeof(DWORD));
        hMapping = CreateOrOpenMapping(name, totalSize, true);
        if (!hMapping) return false;

        view = MapView(hMapping, FILE_MAP_WRITE);
        if (!view) return false;

        DWORD dataSize = static_cast<DWORD>(data.size());
        memcpy(view, &dataSize, sizeof(DWORD));
        memcpy(static_cast<uint8_t*>(view) + sizeof(DWORD), data.data(), data.size());

        //Cleanup(hMapping, view);
        return true;

    }
    catch (...) {
        Cleanup(hMapping, view);
        return false;
    }
}

std::vector<uint8_t> SharedMemory::Read(const std::string& name) {
    HANDLE hMapping = nullptr;
    void* view = nullptr;

    try {
        hMapping = CreateOrOpenMapping(name, 0, false);
        if (!hMapping) return {};

        view = MapView(hMapping, FILE_MAP_READ);
        if (!view) return {};

        DWORD dataSize;
        memcpy(&dataSize, view, sizeof(DWORD));

        if (dataSize == 0) {
            Cleanup(hMapping, view);
            return {};
        }

        std::vector<uint8_t> result(dataSize);
        memcpy(result.data(), static_cast<uint8_t*>(view) + sizeof(DWORD), dataSize);

        //Cleanup(hMapping, view);
        return result;

    }
    catch (...) {
        Cleanup(hMapping, view);
        return {};
    }
}

bool SharedMemory::WriteString(const std::string& name, const std::string& str) {
    std::vector<uint8_t> data(str.begin(), str.end());
    return Write(name, data);
}

std::string SharedMemory::ReadString(const std::string& name, const std::string& defaultValue) {
    auto data = Read(name);
    if (data.empty()) return defaultValue;

    return std::string(data.begin(), data.end());
}

bool SharedMemory::Exists(const std::string& name) {
    HANDLE hMapping = OpenFileMappingA(FILE_MAP_READ, FALSE, name.c_str());
    if (hMapping) {
        CloseHandle(hMapping);
        return true;
    }
    return false;
}

size_t SharedMemory::GetSize(const std::string& name) {
    HANDLE hMapping = OpenFileMappingA(FILE_MAP_READ, FALSE, name.c_str());
    if (!hMapping) return 0;

    void* view = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (!view) {
        CloseHandle(hMapping);
        return 0;
    }

    DWORD dataSize;
    memcpy(&dataSize, view, sizeof(DWORD));

    UnmapViewOfFile(view);
    CloseHandle(hMapping);

    return dataSize;
}

HANDLE SharedMemory::CreateOrOpenMapping(const std::string& name, DWORD size, bool create) {
    if (create) {
        return CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            size,
            name.c_str()
        );
    }
    else {
        return OpenFileMappingA(
            FILE_MAP_READ,
            FALSE,
            name.c_str()
        );
    }
}

void* SharedMemory::MapView(HANDLE hMapping, DWORD access) {
    return MapViewOfFile(hMapping, access, 0, 0, 0);
}

void SharedMemory::Cleanup(HANDLE hMapping, void* view) {
    if (view) UnmapViewOfFile(view);
    if (hMapping) CloseHandle(hMapping);
}