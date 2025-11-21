// SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "utils.h"

namespace mmser {

template <typename T>
struct vector : std::span<T const> {
    using Parent = std::span<T const>;
    std::vector<T> owningBuffer; // only in use if this struct actually owns the data

    void serialize(this auto&& self, auto& ar) {
        if constexpr (ar.loading()) {
            auto data = ar.loadMMap();
            auto data2 = std::span{reinterpret_cast<T const*>(data.data()), data.size()/sizeof(T)};
            self.owningBuffer.resize(data2.size());
            for (size_t i{0}; i < data2.size(); ++i) {
                self.owningBuffer[i] = data2[i];
            }
            self.rebuild();
        } else if constexpr (ar.loadingMMap()) {
            self.owningBuffer.clear();
            auto data = ar.loadMMap(alignof(T));
            auto data2 = std::span{reinterpret_cast<T const*>(data.data()), data.size()/sizeof(T)};
            static_cast<Parent&>(self).operator=(data2);
        } else if constexpr (ar.saving()) {
            auto data = std::span{reinterpret_cast<char const*>(self.data()), self.size()*sizeof(T)};
            ar.saveMMap(data, alignof(T));
        } else {
            auto data = std::span{reinterpret_cast<char const*>(self.data()), self.size()*sizeof(T)};
            ar.storeSizeMMap(data, alignof(T));
        }
    }
    using Parent::size;
    using Parent::operator[];

    void rebuild() {
        auto data = std::span<T const>{owningBuffer.data(), owningBuffer.size()};
        Parent::operator=(data);
    }
    void makeOwning() {
        if (owningBuffer.size() > 0) return;
        if (size() == 0) return;
        owningBuffer.reserve(size());
        for (size_t i{0}; i < size(); ++i) {
            owningBuffer.push_back((*this)[i]);
        }
        rebuild();
    }

    void push_back(T const& t) {
        makeOwning();
        owningBuffer.push_back(t);
        rebuild();
    }

};

}
