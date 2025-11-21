// SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "Mode.h"

#include <cassert>
#include <filesystem>
#include <span>
#include <type_traits>

namespace mmser {

template <Mode _mode>
struct Archive {
    static constexpr Mode mode = _mode;
};

auto requiredPaddingBytes(size_t totalSize, size_t alignment) -> size_t;
template <typename Ar, typename T>
void handle(Ar& ar, T& t);

template <Mode _mode>
struct ArchiveBase {
    static constexpr Mode mode = _mode;
    static constexpr bool loading() { return mode == Mode::Load; }
    static constexpr bool loadingMMap() { return mode == Mode::LoadMMap; }
    static constexpr bool saving() { return mode == Mode::Save; }

    template <typename Self, typename T>
    void operator&(this Self&& self, T&& t) {
        handle(std::forward<Self>(self), std::forward<T>(t));
    }
    template <typename Self, typename ...Args>
    void operator()(this Self&& self, Args&&... args) {
        (handle(std::forward<Self>(self), std::forward<Args>(args)), ...);
    }
};

template <>
struct Archive<Mode::Load> : ArchiveBase<Mode::Load> {
    std::span<char const> buffer;
    size_t totalSize{};

    Archive(std::span<char const> _buffer) : buffer{_buffer} {}

    void load(std::span<char> _in, size_t alignment = 1) {
        auto paddingBytes = requiredPaddingBytes(totalSize, alignment);
        assert(paddingBytes <= buffer.size());
        buffer = buffer.subspan(paddingBytes);

        assert (_in.size() <= buffer.size());
        for (size_t i{}; i < _in.size(); ++i) {
            _in[i] = buffer[i];
        }
        buffer = buffer.subspan(_in.size());
        totalSize = _in.size() + paddingBytes;
    }

    auto loadMMap(size_t alignment = 1) -> std::span<char const> {
        size_t size{};
        *this & size;

        auto paddingBytes = requiredPaddingBytes(totalSize, alignment);
        assert(paddingBytes <= buffer.size());
        buffer = buffer.subspan(paddingBytes);

        assert(size <= buffer.size());
        auto r = buffer.subspan(0, size);
        buffer = buffer.subspan(size);
        totalSize = size + paddingBytes;
        return r;
    }
};

template <>
struct Archive<Mode::LoadMMap> : ArchiveBase<Mode::LoadMMap> {
    std::span<char const> buffer;
    size_t totalSize{};

    Archive(std::span<char const> _buffer) : buffer{_buffer} {}

    void load(std::span<char> _in, size_t alignment = 1) {
        auto paddingBytes = requiredPaddingBytes(totalSize, alignment);
        assert(paddingBytes <= buffer.size());
        buffer = buffer.subspan(paddingBytes);

        assert (_in.size() <= buffer.size());
        for (size_t i{}; i < _in.size(); ++i) {
            _in[i] = buffer[i];
        }
        buffer = buffer.subspan(_in.size());
        totalSize = _in.size() + paddingBytes;
    }

    auto loadMMap(size_t alignment = 1) -> std::span<char const> {
        size_t size{};
        *this & size;

        auto paddingBytes = requiredPaddingBytes(totalSize, alignment);
        assert(paddingBytes <= buffer.size());
        buffer = buffer.subspan(paddingBytes);

        assert(size <= buffer.size());
        auto r = buffer.subspan(0, size);
        buffer = buffer.subspan(size);
        totalSize = size + paddingBytes;
        return r;
    }


};

template <>
struct Archive<Mode::Save> : ArchiveBase<Mode::Save> {
    std::span<char> buffer;
    size_t totalSize{};

    Archive(std::span<char> _buffer) : buffer{_buffer} {}

    void save(std::span<char const> _out, size_t alignment = 1) {
        auto paddingBytes = requiredPaddingBytes(totalSize, alignment);
        assert(paddingBytes <= buffer.size());
        buffer = buffer.subspan(paddingBytes);

        assert (_out.size() <= buffer.size());
        for (size_t i{}; i < _out.size(); ++i) {
            buffer[i] = _out[i];
        }
        buffer = buffer.subspan(_out.size());
        totalSize = _out.size() + paddingBytes;
    }
    void saveMMap(std::span<char const> _out, size_t alignment = 1) {
        auto size = _out.size();
        *this & size;
        save(_out, alignment);
    }
};

template <>
struct Archive<Mode::SaveSize> : ArchiveBase<Mode::SaveSize> {
    size_t totalSize{}; // accumulated size of all elements
    void storeSize(size_t size, size_t alignment = 1) {
        auto paddingBytes = requiredPaddingBytes(totalSize, alignment);
        totalSize += size + paddingBytes;
    }
    void storeSizeMMap(std::span<char const> _out, size_t alignment = 1) {
        auto size = _out.size();
        *this & size;
        storeSize(_out.size(), alignment);
    }
};
}
