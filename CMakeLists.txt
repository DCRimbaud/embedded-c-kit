cmake_minimum_required(VERSION 3.16)
project(embedded_c_kit VERSION 0.3.0 LANGUAGES C)

add_library(ringbuf STATIC src/ring.c)
target_include_directories(ringbuf
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
target_compile_features(ringbuf PUBLIC c_std_99)

if(MSVC)
    target_compile_options(ringbuf PRIVATE /W4)
else()
    target_compile_options(ringbuf PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(mmio STATIC src/mmio.c)
target_include_directories(mmio PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_compile_features(mmio PUBLIC c_std_99)
if(MSVC)
    target_compile_options(mmio PRIVATE /W4)
else()
    target_compile_options(mmio PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(txarb STATIC src/tx_arbiter.c)
target_include_directories(txarb PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_compile_features(txarb PUBLIC c_std_99)
if(MSVC)
    target_compile_options(txarb PRIVATE /W4)
else()
    target_compile_options(txarb PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(bufpool STATIC src/buf_pool.c)
target_include_directories(bufpool PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_compile_features(bufpool PUBLIC c_std_99)
if(MSVC)
    target_compile_options(bufpool PRIVATE /W4)
else()
    target_compile_options(bufpool PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(log STATIC src/log.c)
target_include_directories(log PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_compile_features(log PUBLIC c_std_99)
if(MSVC)
    target_compile_options(log PRIVATE /W4)
else()
    target_compile_options(log PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(xfer STATIC src/xfer.c src/xfer_queue.c)
target_include_directories(xfer PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_compile_features(xfer PUBLIC c_std_99)
if(MSVC)
    target_compile_options(xfer PRIVATE /W4)
else()
    target_compile_options(xfer PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(embedded_c_kit INTERFACE)
target_link_libraries(embedded_c_kit INTERFACE ringbuf mmio txarb bufpool log xfer bytes)
target_include_directories(embedded_c_kit INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)

add_library(bytes STATIC src/bytes.c)
target_include_directories(bytes PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_compile_features(bytes PUBLIC c_std_99)
if(MSVC)
    target_compile_options(bytes PRIVATE /W4)
else()
    target_compile_options(bytes PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(test_ring tests/test_ring.c)
    target_link_libraries(test_ring PRIVATE ringbuf)
    add_test(NAME ring COMMAND test_ring)
    add_executable(test_mmio tests/test_mmio.c)
    target_link_libraries(test_mmio PRIVATE mmio)
    add_test(NAME mmio COMMAND test_mmio)
    add_executable(test_tx_arbiter tests/test_tx_arbiter.c)
    target_link_libraries(test_tx_arbiter PRIVATE txarb)
    add_test(NAME tx_arbiter COMMAND test_tx_arbiter)
    add_executable(test_buf_pool tests/test_buf_pool.c)
    target_link_libraries(test_buf_pool PRIVATE bufpool)
    add_test(NAME buf_pool COMMAND test_buf_pool)
    add_executable(test_log tests/test_log.c)
    target_link_libraries(test_log PRIVATE log)
    add_test(NAME log COMMAND test_log)
    add_executable(test_xfer tests/test_xfer.c)
    target_link_libraries(test_xfer PRIVATE xfer)
    add_test(NAME xfer COMMAND test_xfer)
    add_executable(test_bytes tests/test_bytes.c)
    target_link_libraries(test_bytes PRIVATE bytes)
    add_test(NAME bytes COMMAND test_bytes)
endif()

include(GNUInstallDirs)
install(TARGETS ringbuf mmio txarb bufpool log xfer bytes embedded_c_kit EXPORT embedded_c_kitTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT embedded_c_kitTargets
    NAMESPACE embedded_c_kit::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/embedded_c_kit)
include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/embedded_c_kitConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/embedded_c_kitConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/embedded_c_kit)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/embedded_c_kitConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/embedded_c_kitConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/embedded_c_kitConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/embedded_c_kit)
option(EMBEDDED_C_KIT_BUILD_XILINX_ADAPTERS "Build optional Xilinx adapter modules" OFF)
set(EMBEDDED_C_KIT_XILINX_BSP_INCLUDE_DIR "" CACHE PATH "Xilinx BSP include directory for adapter builds")

if(EMBEDDED_C_KIT_BUILD_XILINX_ADAPTERS)
    if(NOT EMBEDDED_C_KIT_XILINX_BSP_INCLUDE_DIR)
        message(FATAL_ERROR "EMBEDDED_C_KIT_XILINX_BSP_INCLUDE_DIR is required when Xilinx adapters are enabled")
    endif()

    add_library(xilinx_uartlite STATIC adapters/xilinx/uartlite/src/xilinx_uartlite.c)
    target_include_directories(xilinx_uartlite
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/adapters/xilinx/uartlite/include>
        PRIVATE ${EMBEDDED_C_KIT_XILINX_BSP_INCLUDE_DIR})
    target_link_libraries(xilinx_uartlite PUBLIC txarb)
    target_compile_features(xilinx_uartlite PUBLIC c_std_99)
    if(MSVC)
        target_compile_options(xilinx_uartlite PRIVATE /W4)
    else()
        target_compile_options(xilinx_uartlite PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    install(TARGETS xilinx_uartlite EXPORT embedded_c_kitTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(DIRECTORY adapters/xilinx/uartlite/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    if(BUILD_TESTING)
        add_executable(test_xilinx_uartlite tests/test_xilinx_uartlite.c)
        target_link_libraries(test_xilinx_uartlite PRIVATE xilinx_uartlite)
        target_include_directories(test_xilinx_uartlite PRIVATE tests/fake_xilinx)
        add_test(NAME xilinx_uartlite COMMAND test_xilinx_uartlite)
    endif()
endif()