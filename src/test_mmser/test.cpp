// SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
// SPDX-License-Identifier: CC0-1.0
#include <catch2/catch_all.hpp>
#include <mmser/mmser.h>


template <typename T>
auto roundTrip(T const& t) {
    auto size = mmser::computeSaveSize(t);
    auto buffer = std::vector<char>{};
    buffer.resize(size);

    mmser::save(buffer, t);

    T t2;
    mmser::load(buffer, t2);
    return t2;
}

TEST_CASE("Tests mmser", "[mmser][trivial_copyable]") {
    int64_t v{10};

    CHECK(8 == mmser::computeSaveSize(v));
    { // check load
        auto buffer = std::array<char, 8>{11, 0, 0, 0, 0, 0, 0, 0};
        size_t v{};
        mmser::load(buffer, v);
        CHECK(v == 11);
    }
    { // check load via mmap
        auto buffer = std::array<char, 8>{11, 0, 0, 0, 0, 0, 0, 0};
        size_t v{};
        mmser::loadMMap(buffer, v);
        CHECK(v == 11);
    }

    { // check save
        auto buffer = std::array<char, 8>{};
        mmser::save(buffer, v);
        CHECK(buffer[0] == 10);
        CHECK(buffer[1] == 0);
        CHECK(buffer[2] == 0);
        CHECK(buffer[3] == 0);
        CHECK(buffer[4] == 0);
        CHECK(buffer[5] == 0);
        CHECK(buffer[6] == 0);
        CHECK(buffer[7] == 0);
    }
}

struct MyStruct_01 {
    int64_t x{};
    void serialize(this auto&& self, auto& ar) {
        ar & self.x;
    }
};
TEST_CASE("Tests mmser", "[mmser][struct]") {
    MyStruct_01 v;

    auto s = mmser::computeSaveSize(v);
    CHECK(s == 8);
}

struct MyStruct_02 {
    int64_t x{};

    void serialize(this auto&& self, auto& ar) {
//        ar & self.x;
    }
};
TEST_CASE("Tests mmser - struct2", "[mmser][struct]") {
    MyStruct_02 v;

    auto s = mmser::computeSaveSize(v);
    CHECK(s == 0);
}

struct MyStruct_03 : std::span<char const> {
    using Parent = std::span<char const>;
    std::vector<char> owningBuffer; // only in use if this struct actually owns the data

    void serialize(this auto&& self, auto& ar) {
        if constexpr (ar.loading()) {
            auto data = ar.loadMMap();
            self.owningBuffer.resize(data.size());
            for (size_t i{0}; i < data.size(); ++i) {
                self.owningBuffer[i] = data[i];
            }
            self.rebuild();
        } else if constexpr (ar.loadingMMap()) {
            self.owningBuffer.clear();
            auto data = ar.loadMMap();
            static_cast<Parent&>(self).operator=(data);
        } else if constexpr (ar.saving()) {
            ar.saveMMap(self);
        } else {
            ar.storeSizeMMap(self);
        }
    }

    void rebuild() {
        auto data = std::span<char const>{owningBuffer.data(), owningBuffer.size()};
        Parent::operator=(data);
    }
};
TEST_CASE("Tests mmser - struct3", "[mmser][struct]") {
    MyStruct_03 v;

    v.owningBuffer.push_back(1);
    v.owningBuffer.push_back(5);
    v.owningBuffer.push_back(6);
    v.rebuild();

    auto s = mmser::computeSaveSize(v);
    CHECK(s == 11);
    { // check load
        auto buffer = std::array<char, 11>{3, 0, 0, 0, 0, 0, 0, 0, 1, 5, 6};
        MyStruct_03 v;
        mmser::load(buffer, v);
        CHECK(v.owningBuffer.size() == 3);
        CHECK(v.size() == 3);
        CHECK(v.data() == v.owningBuffer.data());
        CHECK(v[0] == 1);
        CHECK(v[1] == 5);
        CHECK(v[2] == 6);
    }
    { // check load via mmap
        auto buffer = std::array<char, 11>{3, 0, 0, 0, 0, 0, 0, 0, 1, 5, 6};
        MyStruct_03 v;
        mmser::loadMMap(buffer, v);
        CHECK(v.owningBuffer.size() == 0);
        CHECK(v.size() == 3);
        CHECK(v[0] == 1);
        CHECK(v[1] == 5);
        CHECK(v[2] == 6);
    }

    { // check save
        auto buffer = std::array<char, 11>{};
        mmser::save(buffer, v);
        CHECK(buffer[0] == 3);
        CHECK(buffer[1] == 0);
        CHECK(buffer[2] == 0);
        CHECK(buffer[3] == 0);
        CHECK(buffer[4] == 0);
        CHECK(buffer[5] == 0);
        CHECK(buffer[6] == 0);
        CHECK(buffer[7] == 0);
        CHECK(buffer[8] == 1);
        CHECK(buffer[9] == 5);
        CHECK(buffer[10] == 6);
    }
}

TEST_CASE("Tests mmser - vector", "[mmser][vector][char]") {
    mmser::vector<char> v;

    v.push_back(1);
    v.push_back(5);
    v.push_back(6);

    auto s = mmser::computeSaveSize(v);
    CHECK(s == 11);
    { // check load
        auto buffer = std::array<char, 11>{3, 0, 0, 0, 0, 0, 0, 0, 1, 5, 6};
        mmser::vector<char> v;
        mmser::load(buffer, v);
        CHECK(v.owningBuffer.size() == 3);
        CHECK(v.size() == 3);
        CHECK(v.data() == v.owningBuffer.data());
        CHECK(v[0] == 1);
        CHECK(v[1] == 5);
        CHECK(v[2] == 6);
    }
    { // check load via mmap
        auto buffer = std::array<char, 11>{3, 0, 0, 0, 0, 0, 0, 0, 1, 5, 6};
        mmser::vector<char> v;
        mmser::loadMMap(buffer, v);
        CHECK(v.owningBuffer.size() == 0);
        CHECK(v.size() == 3);
        CHECK(v[0] == 1);
        CHECK(v[1] == 5);
        CHECK(v[2] == 6);
    }

    { // check save
        auto buffer = std::array<char, 11>{};
        mmser::save(buffer, v);
        CHECK(buffer[0] == 3);
        CHECK(buffer[1] == 0);
        CHECK(buffer[2] == 0);
        CHECK(buffer[3] == 0);
        CHECK(buffer[4] == 0);
        CHECK(buffer[5] == 0);
        CHECK(buffer[6] == 0);
        CHECK(buffer[7] == 0);
        CHECK(buffer[8] == 1);
        CHECK(buffer[9] == 5);
        CHECK(buffer[10] == 6);
    }
}

TEST_CASE("Tests mmser - vector", "[mmser][vector][int16_t]") {
    mmser::vector<int16_t> v;

    v.push_back(1);
    v.push_back(5);
    v.push_back(6);

    auto s = mmser::computeSaveSize(v);
    CHECK(s == 14);
    { // check load
        auto buffer = std::array<char, 14>{6, 0, 0, 0, 0, 0, 0, 0, 1, 0, 5, 0, 6, 0};
        mmser::vector<int16_t> v;
        mmser::load(buffer, v);
        CHECK(v.owningBuffer.size() == 3);
        CHECK(v.size() == 3);
        CHECK(v.data() == v.owningBuffer.data());
        CHECK(v[0] == 1);
        CHECK(v[1] == 5);
        CHECK(v[2] == 6);
    }
    { // check load via mmap
        auto buffer = std::array<char, 14>{6, 0, 0, 0, 0, 0, 0, 0, 1, 0, 5, 0, 6, 0};
        mmser::vector<int16_t> v;
        mmser::loadMMap(buffer, v);
        CHECK(v.owningBuffer.size() == 0);
        CHECK(v.size() == 3);
        CHECK(v[0] == 1);
        CHECK(v[1] == 5);
        CHECK(v[2] == 6);
    }

    { // check save
        auto buffer = std::array<char, 14>{};
        mmser::save(buffer, v);
        CHECK(buffer[ 0] == 6);
        CHECK(buffer[ 1] == 0);
        CHECK(buffer[ 2] == 0);
        CHECK(buffer[ 3] == 0);
        CHECK(buffer[ 4] == 0);
        CHECK(buffer[ 5] == 0);
        CHECK(buffer[ 6] == 0);
        CHECK(buffer[ 7] == 0);
        CHECK(buffer[ 8] == 1);
        CHECK(buffer[ 9] == 0);
        CHECK(buffer[10] == 5);
        CHECK(buffer[11] == 0);
        CHECK(buffer[12] == 6);
        CHECK(buffer[13] == 0);
    }
}

TEST_CASE("Tests mmser", "[mmser][std::array]") {
    std::array<uint16_t, 3> v{1, 5, 6};

    auto s = mmser::computeSaveSize(v);
    CHECK(s == 6);
    { // check load
        auto buffer = std::array<char, 6>{1, 0, 5, 0, 6, 0};
        std::array<uint16_t, 3> v;
        mmser::load(buffer, v);
        CHECK(v[0] == 1);
        CHECK(v[1] == 5);
        CHECK(v[2] == 6);
    }
    { // check load via mmap
        auto buffer = std::array<char, 6>{1, 0, 5, 0, 6, 0};
        std::array<uint16_t, 3> v;
        mmser::loadMMap(buffer, v);
        CHECK(v[0] == 1);
        CHECK(v[1] == 5);
        CHECK(v[2] == 6);
    }

    { // check save
        auto buffer = std::array<char, 6>{};
        mmser::save(buffer, v);
        CHECK(buffer[0] == 1);
        CHECK(buffer[1] == 0);
        CHECK(buffer[2] == 5);
        CHECK(buffer[3] == 0);
        CHECK(buffer[4] == 6);
        CHECK(buffer[5] == 0);
    }
}

TEST_CASE("Tests mmser", "[mmser][std::vector]") {
    std::vector<uint16_t> v{1, 5, 6};

    auto s = mmser::computeSaveSize(v);
    CHECK(s == 14);
    { // check load
        auto buffer = std::array<char, 14>{3, 0, 0, 0, 0, 0, 0, 0, 1, 0, 5, 0, 6, 0};
        std::vector<uint16_t> v;
        mmser::load(buffer, v);
        CHECK(v[0] == 1);
        CHECK(v[1] == 5);
        CHECK(v[2] == 6);
    }
    { // check load via mmap
        auto buffer = std::array<char, 14>{3, 0, 0, 0, 0, 0, 0, 0, 1, 0, 5, 0, 6, 0};
        std::vector<uint16_t> v;
        mmser::loadMMap(buffer, v);
        CHECK(v[0] == 1);
        CHECK(v[1] == 5);
        CHECK(v[2] == 6);
    }

    { // check save
        auto buffer = std::array<char, 14>{};
        mmser::save(buffer, v);
        CHECK(buffer[ 0] == 3);
        CHECK(buffer[ 1] == 0);
        CHECK(buffer[ 2] == 0);
        CHECK(buffer[ 3] == 0);
        CHECK(buffer[ 4] == 0);
        CHECK(buffer[ 5] == 0);
        CHECK(buffer[ 6] == 0);
        CHECK(buffer[ 7] == 0);
        CHECK(buffer[ 8] == 1);
        CHECK(buffer[ 9] == 0);
        CHECK(buffer[10] == 5);
        CHECK(buffer[11] == 0);
        CHECK(buffer[12] == 6);
        CHECK(buffer[13] == 0);
    }
}

TEST_CASE("Tests mmser", "[mmser][std::string]") {
    std::string v{"hello world!"};

    auto s = mmser::computeSaveSize(v);
    CHECK(s == 20);
    { // check load
        auto buffer = std::array<char, 20>{12, 0, 0, 0, 0, 0, 0, 0, 'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!'};
        std::string v;
        mmser::load(buffer, v);
        CHECK(v == "hello world!");
    }
    { // check load via mmap
        auto buffer = std::array<char, 20>{12, 0, 0, 0, 0, 0, 0, 0, 'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!'};
        std::string v;
        mmser::loadMMap(buffer, v);
        CHECK(v == "hello world!");
    }

    { // check save
        auto buffer = std::array<char, 20>{};
        mmser::save(buffer, v);
        CHECK(buffer[ 0] == 12);
        CHECK(buffer[ 1] == 0);
        CHECK(buffer[ 2] == 0);
        CHECK(buffer[ 3] == 0);
        CHECK(buffer[ 4] == 0);
        CHECK(buffer[ 5] == 0);
        CHECK(buffer[ 6] == 0);
        CHECK(buffer[ 7] == 0);
        CHECK(buffer[ 8] == 'h');
        CHECK(buffer[ 9] == 'e');
        CHECK(buffer[10] == 'l');
        CHECK(buffer[11] == 'l');
        CHECK(buffer[12] == 'o');
        CHECK(buffer[13] == ' ');
        CHECK(buffer[14] == 'w');
        CHECK(buffer[15] == 'o');
        CHECK(buffer[16] == 'r');
        CHECK(buffer[17] == 'l');
        CHECK(buffer[18] == 'd');
        CHECK(buffer[19] == '!');
    }
}

TEST_CASE("Tests mmser", "[mmser][std::tuple]") {
    std::tuple<uint16_t, uint8_t> v{1, 5};

    auto s = mmser::computeSaveSize(v);
    CHECK(s == 3);
    { // check load
        auto buffer = std::array<char, 4>{1, 0, 5, 0};
        std::tuple<uint16_t, uint8_t> v;
        mmser::load(buffer, v);
        CHECK(std::get<0>(v) == 1);
        CHECK(std::get<1>(v) == 5);
    }
    { // check load via mmap
        auto buffer = std::array<char, 4>{1, 0, 5, 0};
        std::tuple<uint16_t, uint8_t> v;
        mmser::loadMMap(buffer, v);
        CHECK(std::get<0>(v) == 1);
        CHECK(std::get<1>(v) == 5);
    }

    { // check save
        auto buffer = std::array<char, 4>{};
        mmser::save(buffer, v);
        CHECK(buffer[0] == 1);
        CHECK(buffer[1] == 0);
        CHECK(buffer[2] == 5);
        CHECK(buffer[3] == 0);
    }
}



TEST_CASE("Tests mmser - vector", "[mmser][vector][int16_t][file]") {
    mmser::vector<int16_t> input;

    input.push_back(1);
    input.push_back(5);
    input.push_back(6);


    {
        auto filename = std::filesystem::temp_directory_path() / std::filesystem::path{"unit_test_mmser_load"};
        mmser::saveFile(filename, input);
        auto [output, storageManager] = mmser::loadFile<mmser::vector<int16_t>>(filename);

        REQUIRE(output.size() == input.size());
        for (size_t i{0}; i < output.size(); ++i) {
            CHECK(output[i] == input[i]);
        }
    }

    {
        auto filename = std::filesystem::temp_directory_path() / std::filesystem::path{"unit_test_mmser_load_copy"};
        mmser::saveFile(filename, input);
        auto [output, storageManager] = mmser::loadFileCopy<mmser::vector<int16_t>>(filename);

        REQUIRE(output.size() == input.size());
        for (size_t i{0}; i < output.size(); ++i) {
            CHECK(output[i] == input[i]);
        }
    }
#ifdef MMSER_MMAP
    {
        auto filename = std::filesystem::temp_directory_path() / std::filesystem::path{"unit_test_mmser_load_mmap"};
        mmser::saveFile(filename, input);
        auto [output, storageManager] = mmser::loadFileMMap<mmser::vector<int16_t>>(filename);

        REQUIRE(output.size() == input.size());
        for (size_t i{0}; i < output.size(); ++i) {
            CHECK(output[i] == input[i]);
        }
    }
#endif

    {
        auto filename = std::filesystem::temp_directory_path() / std::filesystem::path{"unit_test_mmser_save"};
        mmser::saveFile(filename, input);
        auto [output, storageManager] = mmser::loadFile<mmser::vector<int16_t>>(filename);

        REQUIRE(output.size() == input.size());
        for (size_t i{0}; i < output.size(); ++i) {
            CHECK(output[i] == input[i]);
        }
    }

    {
        auto filename = std::filesystem::temp_directory_path() / std::filesystem::path{"unit_test_mmser_save_copy"};
        mmser::saveFileCopy(filename, input);
        auto [output, storageManager] = mmser::loadFile<mmser::vector<int16_t>>(filename);

        REQUIRE(output.size() == input.size());
        for (size_t i{0}; i < output.size(); ++i) {
            CHECK(output[i] == input[i]);
        }
    }

#ifdef MMSER_MMAP
    {
        auto filename = std::filesystem::temp_directory_path() / std::filesystem::path{"unit_test_mmser_save_mmap"};
        mmser::saveFileMMap(filename, input);
        auto [output, storageManager] = mmser::loadFile<mmser::vector<int16_t>>(filename);

        REQUIRE(output.size() == input.size());
        for (size_t i{0}; i < output.size(); ++i) {
            CHECK(output[i] == input[i]);
        }
    }
#endif
}
