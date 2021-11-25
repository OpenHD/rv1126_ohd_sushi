#!/bin/bash
##
## Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
## Use of this source code is governed by a BSD-style license that can be
## found in the LICENSE file.
##

find . -name "*.cc" -o -name "*.h" -o -name "*.hh" | xargs clang-format-8 -style=llvm -i
