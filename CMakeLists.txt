# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.22)

include(cmake/ProjectIdentity.cmake)
project(${KFACEAUTH_PROJECT_ID} VERSION ${KFACEAUTH_VERSION} LANGUAGES CXX)

set(QT_MIN_VERSION "6.8.0")
set(KF_MIN_VERSION "6.10.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH "${ECM_MODULE_PATH}")

include(CTest)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Concurrent Core Multimedia Qml Quick)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS CoreAddons I18n KCMUtils)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(PkgConfig REQUIRED)
pkg_check_modules(UDev REQUIRED IMPORTED_TARGET libudev)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/po")
    ki18n_install(po)
endif()

add_subdirectory(engine)
add_subdirectory(src)
add_subdirectory(data)

if(BUILD_TESTING)
    find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Test)
    add_subdirectory(tests/unit)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
