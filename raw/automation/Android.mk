# Copyright (C) 2010 The Android Open Source Project
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

SAMPLE_PATH := $(call my-dir)/../../src
GAMEPLAY_DEPS := ../external/GamePlay/external-deps/lib/android/$(TARGET_ARCH_ABI)

# gameplay
LOCAL_PATH := ../android/gameplay/obj/local/$(TARGET_ARCH_ABI)
include $(CLEAR_VARS)
LOCAL_MODULE    := libgameplay
LOCAL_SRC_FILES := libgameplay.so
include $(PREBUILT_SHARED_LIBRARY)

# gameobjects
LOCAL_PATH :=  ../external/gameobjects-gameplay3d/android/obj/local/$(TARGET_ARCH_ABI)
include $(CLEAR_VARS)
LOCAL_MODULE    := libgameobjects
LOCAL_SRC_FILES := libgameobjects.a
include $(PREBUILT_STATIC_LIBRARY)

# libgameplay-deps
LOCAL_PATH := $(GAMEPLAY_DEPS)
include $(CLEAR_VARS)
LOCAL_MODULE    := libgameplay-deps
LOCAL_SRC_FILES := libgameplay-deps.a
include $(PREBUILT_STATIC_LIBRARY)

# platformer
LOCAL_PATH := $(SAMPLE_PATH)
include $(CLEAR_VARS)

LOCAL_MODULE    := platformer-sample
LOCAL_SRC_FILES :=  ../external/GamePlay/gameplay/src/gameplay-main-android.cpp \
###INSERT_AUTO_GENERATED_CPP_LIST###

LOCAL_CPPFLAGS += -std=c++11 -Wno-switch-enum -Wno-switch
LOCAL_ARM_MODE := arm
LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv2 -lOpenSLES
LOCAL_CFLAGS    := -D__ANDROID__ -DGP_NO_LUA_BINDINGS -I"../external/GamePlay/external-deps/include" -I"../external/GamePlay/gameplay/src" -I"../external/gameobjects-gameplay3d/src"

LOCAL_STATIC_LIBRARIES := android_native_app_glue libgameplay-deps gameobjects
LOCAL_SHARED_LIBRARIES := gameplay

include $(BUILD_SHARED_LIBRARY)
$(call import-module,android/native_app_glue)
