#
# Copyright 2022 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at:
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

INCLUDE += $(SRC_DIR)/include

liblc3_src += \
    $(SRC_DIR)/attdet/attdet.c \
    $(SRC_DIR)/bits/bits.c \
    $(SRC_DIR)/bwdet/bwdet.c \
    $(SRC_DIR)/energy/energy.c \
    $(SRC_DIR)/ltpf/ltpf.c \
    $(SRC_DIR)/mdct/mdct.c \
    $(SRC_DIR)/plc/plc.c \
    $(SRC_DIR)/sns/sns.c \
    $(SRC_DIR)/spec/spec.c \
    $(SRC_DIR)/tables/tables.c \
    $(SRC_DIR)/tns/tns.c\
    $(SRC_DIR)/header/header.c\
    $(SRC_DIR)/bytestream/bytestream.c\
    $(SRC_DIR)/wave/wave.c\
    $(SRC_DIR)/lc3bin.c\
    $(SRC_DIR)/lc3.c\
    $(SRC_DIR)/file_coder.c\
    $(SRC_DIR)/stream_coder.c

liblc3_cflags += -ffast-math

$(eval $(call add-lib,liblc3))

default: liblc3

$(eval $(call add-so,lc3so))
lc3so_src += $(liblc3_src)
lc3so_cflags += $(liblc3_cflags) -fPIC

.PHONY: lc3so
lc3so:
