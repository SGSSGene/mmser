// SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "../Handler.h"
#include "span.h"

#include <string>

namespace mmser {

template <>
struct Handler<std::string> {
    static void serialize(auto& t, auto& ar) {
        uint64_t s = t.size();
        ar(s);

        if constexpr (ar.loading() || ar.loadingMMap()) {
            t.resize(s);
        }

        auto buffer = std::span{t.data(), t.size()};
        ar(buffer);
    }
};

}
