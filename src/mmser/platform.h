// SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
// SPDX-License-Identifier: AGPL-3.0-or-later


// Flags that are required for specific platforms
#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER) && !defined(__INTEL_LLVM_COMPILER)
    #define MMSER_IGNORE_GCC_FLAG_BUG1
#endif
