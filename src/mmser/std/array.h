// SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "../Handler.h"
#include "span.h"

#include <array>

namespace mmser {

template <typename TEntry, size_t N>
struct Handler<std::array<TEntry, N>> {
    static void serialize(auto& t, auto& ar) {
        auto s = std::span{t.data(), t.size()};
        ar(s);
    }
};

}
