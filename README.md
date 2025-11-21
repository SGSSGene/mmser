
<!--
    SPDX-FileCopyrightText: 2025 Simon Gene Gottlieb
    SPDX-License-Identifier: CC-BY-4.0
-->

# mmap-ser

A C++ class/struct serializer which is optimized to take advantages of mmap for trivial data types.
It focuses on Linux OS and offers basic compatibilities such that structures can be written/read from on other OS.



## Limitations
Things that this can not do and is not a goal:
 - Platform independent data format (little/big endian)
