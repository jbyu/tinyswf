# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# the purpose of this sample is to demonstrate how one can
# generate two distinct shared libraries and have them both
# uploaded in
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libtinyswf

MY_PREFIX := $(LOCAL_PATH)/../../../

MY_SOURCES = $(wildcard $(MY_PREFIX)/src/*.cpp) \
	$(wildcard $(MY_PREFIX)/src/tags/*.cpp) \
	$(wildcard $(MY_PREFIX)/libtess2/*.c)

LOCAL_SRC_FILES := $(subst jni/, , $(MY_SOURCES))

LOCAL_C_INCLUDES := $(MY_PREFIX)/include \
	$(MY_PREFIX)/libtess2 \
	$(MY_PREFIX)/rapidxml \
	$(MY_PREFIX)/src/tags

LOCAL_CFLAGS := -std=c++11 -fexceptions -frtti -Wno-literal-suffix

include $(BUILD_STATIC_LIBRARY)
