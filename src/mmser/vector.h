// SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "utils.h"

#include <initializer_list>

namespace mmser {

template <typename T>
struct vector {
    std::span<T const> view;     // view on the data, either on a mmap or on owningBuffer
    std::vector<T> owningBuffer; // only in use if this struct actually owns the data

    vector() = default;
    vector(size_t _size)
        : owningBuffer(_size)
    {
        rebuild();
    }
    vector(size_t _size, T const& v)
        : owningBuffer(_size, v)
    {
        rebuild();
    }

    vector(vector const& _oth) {
        *this = _oth;
    }
    vector(vector&& _oth) {
        *this = std::move(_oth);
    }

    vector(std::initializer_list<T> list)
        : owningBuffer{list}
    {
        rebuild();
    }

    auto operator=(vector const& _oth) -> auto& {
        owningBuffer.assign(_oth.view.begin(), _oth.view.end());
        rebuild();
        return *this;
    }
    auto operator=(vector&& _oth) -> auto& {
        if (_oth.size() == 0) return *this;
        if (_oth.owningBuffer.size() == 0) {
            owningBuffer = std::move(_oth.owningBuffer);
            view = _oth.view;
            return *this;
        }
        owningBuffer = std::move(_oth.owningBuffer);
        rebuild();
        return *this;
    }

    template <typename Ar>
    void serialize(this auto&& self, Ar& ar) {
        if constexpr (is_mmser<std::remove_cvref_t<Ar>>) {
            if constexpr (Ar::loading()) {
                auto data = ar.loadMMap(alignof(T));
                auto data2 = std::span{reinterpret_cast<T const*>(data.data()), data.size()/sizeof(T)};
                self.owningBuffer.resize(data2.size());
                for (size_t i{0}; i < data2.size(); ++i) {
                    self.owningBuffer[i] = data2[i];
                }
                self.rebuild();
            } else if constexpr (Ar::loadingMMap()) {
                self.owningBuffer.clear();
                auto data = ar.loadMMap(alignof(T));
                auto data2 = std::span{reinterpret_cast<T const*>(data.data()), data.size()/sizeof(T)};
                self.view = data2;
            } else if constexpr (Ar::saving()) {
                auto data = std::span{reinterpret_cast<char const*>(self.view.data()), self.size()*sizeof(T)};
                ar.saveMMap(data, alignof(T));
            } else {
                auto data = std::span{reinterpret_cast<char const*>(self.view.data()), self.size()*sizeof(T)};
                ar.storeSizeMMap(data, alignof(T));
            }
        } else {
            self.makeOwning();
            ar(self.owningBuffer);
            self.rebuild();
        }
    }

    auto size() const {
        return view.size();
    }

    auto operator[](size_t idx) const -> auto const& {
        return view[idx];
    }
    auto operator[](size_t idx) -> auto& {
        makeOwning();
        return owningBuffer[idx];
    }

    void rebuild() {
        view = {owningBuffer.data(), owningBuffer.size()};
    }
    void makeOwning() {
        if (owningBuffer.size() > 0) return;
        if (size() == 0) return;
        owningBuffer.reserve(size());
        for (size_t i{0}; i < size(); ++i) {
            owningBuffer.push_back(view[i]);
        }
        rebuild();
    }

    void push_back(T const& t) {
        makeOwning();
        owningBuffer.push_back(t);
        rebuild();
    }
    template <typename ...Args>
    void emplace_back(Args&& ...args) {
        makeOwning();
        owningBuffer.emplace_back(std::forward<Args>(args)...);
        rebuild();
    }

    void resize(size_t s) {
        makeOwning();
        owningBuffer.resize(s);
        rebuild();
    }

    void resize(size_t s, T const& v) {
        makeOwning();
        owningBuffer.resize(s, v);
        rebuild();
    }

    void reserve(size_t s) {
        makeOwning();
        owningBuffer.reserve(s);
        rebuild();
    }

    auto back() -> auto& {
        makeOwning();
        return owningBuffer.back();
    }

    auto back() const -> auto const& {
        return view.back();
    }
};

}
