// SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "../Handler.h"
#include "span.h"

#include <array>

namespace mmser {

template <typename... Ts>
struct Handler<std::tuple<Ts...>> {

    template <size_t I, typename CB>
    static void for_constexpr(CB const& cb) {
        if constexpr (I < sizeof...(Ts)) {
            cb.template operator()<I>();
            for_constexpr<I+1>(cb);
        }
    }

    static void serialize(auto& t, auto& ar) {
        for_constexpr<0>([&]<size_t I>() {
            ar(std::get<I>(t));
        });
    }
};

}
