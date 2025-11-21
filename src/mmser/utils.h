// SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "Archive.h"
#include "Handler.h"

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
}
