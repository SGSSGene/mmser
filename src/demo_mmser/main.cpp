// SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
// SPDX-License-Identifier: CC0-1.0
#include <mmser/mmser.h>

int main(int argc, char** args) {
    if (argc < 2)  return 1;

    auto const path = std::filesystem::path{"tmp.idx"};

    if (std::string{"load"} == args[1]) {
        auto [buffer, sm] = mmser::loadFile<mmser::vector<uint64_t>>(path);
        (void)buffer;
    } else if (std::string{"load_and_run"} == args[1]) {
        auto [buffer, sm] = mmser::loadFile<mmser::vector<uint64_t>>(path);
        size_t total{};
        for (size_t i{0}; i < buffer.size(); ++i) {
            total += buffer[i];
        }
        (void)total;

    } else if (std::string{"save"} == args[1]) {
        auto buffer = mmser::vector<uint64_t>{};
        buffer.resize(500'000'000, 1);
        mmser::saveFileStream(path, buffer);
//        mmser::saveFile(path, buffer);
    } else {
        throw std::runtime_error{"unknown command"};
    }
}
