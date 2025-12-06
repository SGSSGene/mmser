// SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "../Handler.h"
#include "../trivially_copyable.h"

#include <span>

namespace mmser {

template <typename TEntry>
struct Handler<std::span<TEntry>> {
    template <typename Ar>
    static void serialize(auto& t, Ar& ar) {
        if constexpr (is_trivially_copyable<TEntry>) {
            if constexpr (Ar::loading() || Ar::loadingMMap()) {
                auto in = std::span<char>{reinterpret_cast<char*>(t.data()), sizeof(TEntry)*t.size()};
                ar.load(in, alignof(TEntry));
            } else if constexpr (Ar::saving()) {
                auto out = std::span<char const>{reinterpret_cast<char const*>(t.data()), sizeof(TEntry)*t.size()};
                ar.save(out, alignof(TEntry));
            } else {
                ar.storeSize(sizeof(TEntry)*t.size(), alignof(TEntry));
            }
        } else {
            for (size_t i{0}; i < t.size(); ++i) {
                ar(t[i]);
            }
        }
    }
};

}
