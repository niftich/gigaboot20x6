#!/bin/bash -e

# Copyright 2016 The Fuchsia Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if [ -z "$1" ]; then
	echo usage: $0 "<diskimg>"
	exit 1
fi

if [[ ! -f $1 ]]; then
	echo creating: $1
	dd if=/dev/zero of="$1" bs=512 count=93750

	parted "$1" -s -a minimal mklabel gpt
	parted "$1" -s -a minimal mkpart EFI FAT16 2048s 93716s
	parted "$1" -s -a minimal toggle 1 boot

	mformat -i "$1"@@1024K -h 32 -t 32 -n 64 -c 1
    mmd -i "$1"@@1024K ::EFI
    mmd -i "$1"@@1024K ::EFI/BOOT
fi

