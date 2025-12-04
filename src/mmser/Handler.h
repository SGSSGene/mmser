// SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "Archive.h"

namespace mmser {

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
    static constexpr bool hasLoadAndSave = requires(T const& t_const, std::remove_cv_t<T>& t_mut) {
        { t_mut.load(ar) };
        { t_const.save(ar) };
        { t_const.saveSize(ar) };
    };
    static constexpr bool hasHandler = requires() {
        { Handler<std::remove_cv_t<T>>::serialize(t, ar) };
    };

    if constexpr (hasSerialize) {
        t.serialize(ar);
    } else if constexpr (hasLoadAndSave) {
        if constexpr (Ar::loading() || Ar::loadingMMap()) {
            t.load(ar);
        } else if constexpr (Ar::saving()) {
            t.save(ar);
        } else {
            t.saveSize(ar);
        }
    } else if constexpr (hasHandler) {
        Handler<std::remove_cv_t<T>>::serialize(t, ar);
    } else {
        []<bool f = false> {
            static_assert(f, "has no valid serialize() overload nor a registered mmser::Handler");
        }();
    }
}

template <typename T>
    requires (
        std::is_trivially_copyable_v<std::remove_const_t<T>>
        && !std::is_class_v<std::remove_const_t<T>>
    )
struct Handler<T> {
    template <typename Ar>
    static void serialize(auto& t, Ar& ar) {
        if constexpr (Ar::loading() || Ar::loadingMMap()) {
            auto in = std::span<char>{reinterpret_cast<char*>(&t), sizeof(t)};
            ar.load(in, alignof(T));
        } else if constexpr (Ar::saving()) {
            auto out = std::span<char const>{reinterpret_cast<char const*>(&t), sizeof(t)};
            ar.save(out, alignof(T));
        } else {
            ar.storeSize(sizeof(t), alignof(T));
        }
    }
};

}
