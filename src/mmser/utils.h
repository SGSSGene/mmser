// SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "Archive.h"
#include "Handler.h"

#include <any>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#if (defined(unix) || defined(__unix__) || defined(__unix))
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <tuple>
    #include <unistd.h>
    #define MMSER_MMAP
#endif


namespace mmser {
inline auto requiredPaddingBytes(size_t totalSize, size_t alignment) -> size_t {
    size_t usedBytesOfNextElement = totalSize % alignment;
    if (usedBytesOfNextElement == 0) return 0;
    return alignment - usedBytesOfNextElement;
}


template <typename T>
void load(std::span<char const> buffer, T& t) {
    auto archive = Archive<Mode::Load>{buffer};
    handle(archive, t);
}

template <typename T>
void loadMMap(std::span<char const> buffer, T& t) {
    auto archive = Archive<Mode::LoadMMap>{buffer};
    handle(archive, t);
}

template <typename T>
void save(std::span<char> buffer, T const& t) {
    auto archive = Archive<Mode::Save>{buffer};
    handle(archive, t);
}

template <typename T>
size_t computeSaveSize(T const& t) {
    auto archive = Archive<Mode::SaveSize>{};
    handle(archive, t);
    return archive.totalSize;
}

using Storage = std::unique_ptr<std::any>;

template <typename T>
auto loadFileCopy(std::filesystem::path const& path) -> std::tuple<T, Storage> {
    auto ret = std::tuple<T, Storage>{};

    auto buffer = std::vector<char>{};
    {
        auto file = std::ifstream{path, std::ios::in | std::ios::binary | std::ios::ate};
        buffer.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), buffer.size());
    }
    load(buffer, std::get<0>(ret));
    return ret;
}

#ifdef MMSER_MMAP
template <typename T>
auto loadFileMMap(std::filesystem::path const& path) -> std::tuple<T, Storage> {
    auto ret = std::tuple<T, Storage>{};

    auto file_fd = ::open(path.c_str(), O_RDONLY);
    if (file_fd == -1) {
        throw std::runtime_error{"file " + path.string() + " not readable"};
    }
    auto file_size = static_cast<size_t>(std::filesystem::file_size(path));
    auto ptr = (char const*)mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
    if (ptr == MAP_FAILED) {
        throw std::runtime_error{"mmap failed"};
    }
    auto buffer = std::span<char const>{ptr, file_size};
    loadMMap(buffer, std::get<0>(ret));
    return ret;
}
#endif


template <typename T>
auto loadFile(std::filesystem::path const& path) -> std::tuple<T, Storage> {
    #ifdef MMSER_MMAP
        return loadFileMMap<T>(path);
    #else
        return loadFileCopy<T>(path);
    #endif
}

template <typename T>
void saveFileCopy(std::filesystem::path const& path, T const& t) {
    auto size = computeSaveSize(t);

    auto buffer = std::vector<char>{};
    buffer.resize(size);
    save(buffer, t);
    {
        auto file = std::ofstream{path, std::ios::out | std::ios::binary | std::ios::trunc};
        file.write(buffer.data(), buffer.size());
    }
}

struct ArchiveStream : ArchiveBase<Mode::Save> {
    std::ofstream ofs;
    size_t totalSize{};

    std::vector<char> paddingBuffer; // reusable buffer to add padding data

    ArchiveStream(std::filesystem::path _path)
        : ofs{_path, std::ios::out | std::ios::binary | std::ios::trunc}
    {}

    void save(std::span<char const> _out, size_t alignment = 1) {
        auto paddingBytes = requiredPaddingBytes(totalSize, alignment);
        paddingBuffer.resize(paddingBytes, 0);
        ofs.write(paddingBuffer.data(), paddingBuffer.size());
        ofs.write(_out.data(), _out.size());

        totalSize += _out.size() + paddingBytes;
    }
    void saveMMap(std::span<char const> _out, size_t alignment = 1) {
        auto size = _out.size();
        *this & size;
        save(_out, alignment);
    }
};

template <>
struct is_mmser_t<ArchiveStream> : std::true_type {};

template <typename T>
void saveFileStream(std::filesystem::path const& path, T const& t) {
    auto archive = ArchiveStream{path};
    handle(archive, t);
}

#ifdef MMSER_MMAP
template <typename T>
void saveFileMMap(std::filesystem::path const& path, T const& t) {
    auto size = computeSaveSize(t);

    auto file_fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (file_fd == -1) {
        throw std::runtime_error{"file " + path.string() + " not writable"};
    }

    if (size > 0) {
        if (auto r = lseek(file_fd, size-1, SEEK_SET); r == -1) {
            close(file_fd);
            throw std::runtime_error{"file " + path.string() + " not writable, ::lseek error"};
        }
        if (auto r = write(file_fd, "", 1); r != 1) {
            close(file_fd);
            throw std::runtime_error{"file " + path.string() + " not writable, ::write error"};
        }
        auto ptr = (char*)mmap(nullptr, size, PROT_WRITE, MAP_SHARED, file_fd, 0);
        if (ptr == MAP_FAILED) {
            close(file_fd);
            throw std::runtime_error{"mmap failed"};
        }
        auto buffer = std::span<char>{ptr, size};
        save(buffer, t);
        if (auto r = munmap((void*)ptr, size); r != 0) {
            throw std::runtime_error{std::string{"munmap failed: "} + strerror(errno) + "(" + std::to_string(errno) + ")"};
        }
    }
    if (auto r = close(file_fd); r != 0) {
        throw std::runtime_error{"::close failed"};
    }
}
#endif

template <typename T>
void saveFile(std::filesystem::path const& path, T const& t) {
    #ifdef MMSER_MMAP
        saveFileMMap(path, t);
    #else
        saveFileCopy(path, t);
    #endif
}
}
