// SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <bitset>
#include <type_traits>

namespace mmser {

template <typename T>
struct is_trivially_copyable_t : std::false_type {};

template <typename T>
concept is_trivially_copyable = is_trivially_copyable_t<T>::value;

template <typename TEntry>
    requires (std::is_trivially_copyable_v<TEntry>)
struct is_trivially_copyable_t<TEntry> : std::true_type {};

template <size_t N>
struct is_trivially_copyable_t<std::bitset<N>> : std::true_type {};

}
