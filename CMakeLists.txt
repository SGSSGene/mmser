# SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
# SPDX-License-Identifier: CC0-1.0
cmake_minimum_required (VERSION 3.31)

project (mmser LANGUAGES CXX)
include(cmake/CPM.cmake)
CPMAddPackage("gh:SGSSGene/cpmpack@1.0.2")
loadCPMPack("${CMAKE_CURRENT_SOURCE_DIR}/cpmpack.json")

if (MMSER_BUILD_TEST)
    enable_testing()
    add_test(NAME test_mmser COMMAND test_mmser)
endif ()
