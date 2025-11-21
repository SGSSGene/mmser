// SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <cassert>
#include <filesystem>
#include <span>
#include <type_traits>

#define MMSER

namespace mmser {
enum class Mode { Load, LoadMMap, Save, SaveSize };

template <Mode _mode>
struct Archive {
    static constexpr Mode mode = _mode;
};

auto requiredPaddingBytes(size_t totalSize, size_t alignment) -> size_t {
    size_t usedBytesOfNextElement = totalSize % alignment;
    if (usedBytesOfNextElement == 0) return 0;
    return alignment - usedBytesOfNextElement;
}

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


template <typename T>
struct Handler;

template <typename Ar, typename T>
void handle(Ar& ar, T& t) {
    static_assert(
        !std::is_pointer_v<T>,
        "Pointers can not be serialized"
    );

    static constexpr bool hasSerialize = requires() {
        { t.serialize(ar) };
    };
    static constexpr bool hasHandler = requires() {
        { Handler<T>{t} };
    };

    if constexpr (hasSerialize) {
        t.serialize(ar);
    } else if constexpr (hasHandler) {
        auto h = Handler<T>{t};
        h.serialize(ar);
    } else {
        []<bool f = false> {
            static_assert(f, "has no valid serialize() overload nor a registered mmser::Handler");
        }();
    }
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

template <typename T>
    requires (
        std::is_trivially_copyable_v<std::remove_const_t<T>>
        && !std::is_class_v<std::remove_const_t<T>>
    )
struct Handler<T> {
    T& t;

    void serialize(auto& ar) {
        if constexpr (ar.loading() || ar.loadingMMap()) {
            auto in = std::span<char>{reinterpret_cast<char*>(&t), sizeof(t)};
            ar.load(in, alignof(T));
        } else if constexpr (ar.saving()) {
            auto out = std::span<char const>{reinterpret_cast<char const*>(&t), sizeof(t)};
            ar.save(out, alignof(T));
        } else {
            ar.storeSize(sizeof(t), alignof(T));
        }
    }
};
}
