cmake_minimum_required(VERSION 3.21)
# The version string is upstream's (xmake.lua set_version), so the Mac build reports what upstream's own build reports.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../xmake.lua" _upstream_xmake)
string(REGEX MATCH "set_version\\(\"([0-9]+\\.[0-9]+\\.[0-9]+)\"" _upstream_version "${_upstream_xmake}")
if(NOT CMAKE_MATCH_1)
    message(FATAL_ERROR "no set_version(\"x.y.z\") in ../xmake.lua")
endif()
project(ArchiveXL VERSION ${CMAKE_MATCH_1} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64")

if(NOT APPLE)
    message(FATAL_ERROR "This target builds the macOS ArchiveXL dylib")
endif()

add_compile_definitions(RED4EXT_PLATFORM_MACOS)
add_compile_definitions(RED4EXT_USE_FRIDA_GADGET)
add_compile_definitions(RED4EXT_HEADER_ONLY)
add_compile_definitions(HOOKCHAIN_SELF=archivexl)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")

# Built from this tree plus the macOS RED4ext SDK checkout (a sibling clone by default). lib/Core and lib/Support are this
# repository's own copies (psiberx's shared libraries with the macOS changes of the Codeware port applied); nameof and semver
# are the pinned submodules.
set(AXL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(RED4EXT_SDK_DIR "${AXL_DIR}/../RED4ext.SDK-macos" CACHE PATH "macOS RED4ext SDK checkout")
set(SDK_INCLUDE "${RED4EXT_SDK_DIR}/include")

# Reproducible builds: no checkout path ends up in the dylib (__FILE__ in asserts and logs), so the same sources build to the same bytes
# from any directory.
get_filename_component(_repo_root "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
get_filename_component(_sdk_root "${RED4EXT_SDK_DIR}" ABSOLUTE)
add_compile_options("-ffile-prefix-map=${_repo_root}=." "-ffile-prefix-map=${_sdk_root}=RED4ext.SDK-macos"
                    "-ffile-prefix-map=${CMAKE_BINARY_DIR}=build")

set(NAME "ArchiveXL")
set(AUTHOR "psiberx")
set(VERSION "${PROJECT_VERSION}")
set(MODE "release")
set(DEBUG "")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/App")
configure_file(
    "${AXL_DIR}/config/Project.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/App/Project.hpp"
)

find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(yaml-cpp REQUIRED)

set(ARCHIVEXL_SOURCES
    Main.cpp
    MacSupport.cpp
    MacAddressResolver.cpp
    MacTrap.cpp
    MacDiag.cpp
    MacDepotDiag.cpp
    MacCrashDiag.cpp
    MacHookChain.cpp
    ../src/App/Application.cpp
    ../src/App/Archives/ArchiveService.cpp
    ../src/App/Extensions/Animation/Config.cpp
    ../src/App/Extensions/Animation/Extension.cpp
    ../src/App/Extensions/Attachment/Extension.cpp
    ../src/App/Extensions/Customization/Config.cpp
    ../src/App/Extensions/Customization/Extension.cpp
    ../src/App/Extensions/ExtensionLoader.cpp
    ../src/App/Extensions/ExtensionService.cpp
    ../src/App/Extensions/FactoryIndex/Config.cpp
    ../src/App/Extensions/FactoryIndex/Extension.cpp
    ../src/App/Extensions/Garment/Config.cpp
    ../src/App/Extensions/Garment/Dynamic.cpp
    ../src/App/Extensions/Garment/Extension.cpp
    ../src/App/Extensions/Garment/Prefix.cpp
    ../src/App/Extensions/Garment/States.cpp
    ../src/App/Extensions/Garment/Tags.cpp
    ../src/App/Extensions/Garment/Wrapper.cpp
    ../src/App/Extensions/InkSpawner/Extension.cpp
    ../src/App/Extensions/Journal/Config.cpp
    ../src/App/Extensions/Journal/Extension.cpp
    ../src/App/Extensions/Localization/Config.cpp
    ../src/App/Extensions/Localization/Extension.cpp
    ../src/App/Extensions/Localization/Language.cpp
    ../src/App/Extensions/Mesh/Extension.cpp
    ../src/App/Extensions/PuppetState/Config.cpp
    ../src/App/Extensions/PuppetState/Extension.cpp
    ../src/App/Extensions/PuppetState/Handler.cpp
    ../src/App/Extensions/PuppetState/System.cpp
    ../src/App/Extensions/QuestPhase/Config.cpp
    ../src/App/Extensions/QuestPhase/Extension.cpp
    ../src/App/Extensions/ResourceLink/Config.cpp
    ../src/App/Extensions/ResourceLink/Extension.cpp
    ../src/App/Extensions/ResourceMeta/Config.cpp
    ../src/App/Extensions/ResourceMeta/Extension.cpp
    ../src/App/Extensions/ResourcePatch/Config.cpp
    ../src/App/Extensions/ResourcePatch/Extension.cpp
    ../src/App/Extensions/Transmog/Extension.cpp
    ../src/App/Extensions/WorldStreaming/Config.cpp
    ../src/App/Extensions/WorldStreaming/Extension.cpp
    ../src/App/Facade.cpp
    ../src/App/Patches/EntitySpawnerPatch.cpp
    ../src/App/Patches/WorldWidgetLimitPatch.cpp
    ../src/App/Shared/ResourcePathRegistry.cpp
    ../src/App/Utils/MacProbe.cpp
    ../src/rtti.cpp
    ${AXL_DIR}/lib/Core/Facades/Runtime.cpp
    ${AXL_DIR}/lib/Core/Runtime/HostImage.cpp
    ${AXL_DIR}/lib/Core/Runtime/ModuleImage.cpp
    ${AXL_DIR}/lib/Core/Hooking/HookingAgent.cpp
    ${AXL_DIR}/lib/Core/Hooking/HookingDriver.cpp
    ${AXL_DIR}/lib/Core/Memory/AddressResolver.cpp
    ${AXL_DIR}/lib/Core/Logging/LoggingAgent.cpp
    ${AXL_DIR}/lib/Core/Logging/LoggingDriver.cpp
    ${AXL_DIR}/lib/Core/Runtime/OwnerMutex.cpp
    ${AXL_DIR}/lib/Core/Foundation/Application.cpp
    ${AXL_DIR}/lib/Core/Foundation/RuntimeProvider.cpp
    ${AXL_DIR}/lib/Support/RedLib/RedLibProvider.cpp
)

# R16a. SDK-internal address lookups (UniversalRelocFunc, for example CClass::GetProperty) resolve through the
# SDK's own Mac table in RED4ext/Relocation-inl.hpp, not through ArchiveXL's MacAddressResolver. The vendored SDK
# (the sibling RED4ext.SDK-macos) is not touched. The rows ArchiveXL needs on top of its table live in MacSdkRows.inc and are
# injected into a generated copy of the header, which comes first on the include path.
set(SDK_RELOC "${SDK_INCLUDE}/RED4ext/Relocation-inl.hpp")
set(SDK_ROWS_INC "${CMAKE_CURRENT_SOURCE_DIR}/MacSdkRows.inc")
set(SDK_OVERLAY "${CMAKE_CURRENT_BINARY_DIR}/sdk-overlay")
file(READ "${SDK_RELOC}" SDK_RELOC_TEXT)
file(READ "${SDK_ROWS_INC}" SDK_ROWS_TEXT)
set(SDK_ROWS_MARKER "        { 4201325776, 0x109BAA8 },  // CGlobalFunction_ctor\n")
string(FIND "${SDK_RELOC_TEXT}" "${SDK_ROWS_MARKER}" SDK_ROWS_POS)
if(SDK_ROWS_POS EQUAL -1)
    message(FATAL_ERROR "R22: the anchor row (CGlobalFunction_ctor) is not in ${SDK_RELOC}. Update the anchor in CMakeLists.txt.")
endif()
string(REPLACE "${SDK_ROWS_MARKER}" "${SDK_ROWS_MARKER}${SDK_ROWS_TEXT}" SDK_RELOC_TEXT "${SDK_RELOC_TEXT}")

# R17. An unmapped hash must not resolve to 0 (the caller would jump to null). Replace the unknown-hash tail of the Mac
# Resolve with a call to the trap table (macos/MacTrap.cpp): the returned stub logs once and returns zero.
set(SDK_TAIL_START "    // Address not found or placeholder - return a non-zero sentinel value")
set(SDK_TAIL_END "    return 0;\n#else\n    if constexpr (Detail::AddressResolverOverride<uint32_t>::value)")
string(FIND "${SDK_RELOC_TEXT}" "${SDK_TAIL_START}" SDK_TAIL_A)
string(FIND "${SDK_RELOC_TEXT}" "${SDK_TAIL_END}" SDK_TAIL_B)
if(SDK_TAIL_A EQUAL -1 OR SDK_TAIL_B EQUAL -1)
    message(FATAL_ERROR "R22: the unknown-hash tail of Resolve was not found in ${SDK_RELOC}. Update the anchors in CMakeLists.txt.")
endif()
string(SUBSTRING "${SDK_RELOC_TEXT}" 0 ${SDK_TAIL_A} SDK_HEAD)
math(EXPR SDK_REST_FROM "${SDK_TAIL_B} + 14")
string(SUBSTRING "${SDK_RELOC_TEXT}" ${SDK_REST_FROM} -1 SDK_REST)
set(SDK_RELOC_TEXT "${SDK_HEAD}    // R22: ArchiveXL trap table. Logs on the first call of the returned stub, then returns 0.\n    return archivexl_mac_trap_for_hash(aHash);\n${SDK_REST}")
set(SDK_DECL_MARKER "extern \"C\" uintptr_t tweakxl_class_vslot(uint32_t aByteOffset);\n")
string(FIND "${SDK_RELOC_TEXT}" "${SDK_DECL_MARKER}" SDK_DECL_POS)
if(SDK_DECL_POS EQUAL -1)
    message(FATAL_ERROR "R22: the extern declaration anchor was not found in ${SDK_RELOC}.")
endif()
string(REPLACE "${SDK_DECL_MARKER}" "${SDK_DECL_MARKER}extern \"C\" uintptr_t archivexl_mac_trap_for_hash(uint32_t aHash);\n" SDK_RELOC_TEXT "${SDK_RELOC_TEXT}")
file(WRITE "${SDK_OVERLAY}/RED4ext/Relocation-inl.hpp" "${SDK_RELOC_TEXT}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${SDK_RELOC}" "${SDK_ROWS_INC}")
message(STATUS "R22: SDK Mac table overlay written to ${SDK_OVERLAY}")

# 2.5: the generated layout guard asserts the Mac reflection-dump layouts. Leave it out (-DMAC_LAYOUT_GUARD=OFF) only for the build
# that takes a new reflection dump; regenerate it with tools/mac/gen-mac-layout.py and build with the guard again afterwards.
option(MAC_LAYOUT_GUARD "Build macos/MacLayoutGuard.cpp (generated layout asserts)" ON)

add_library(ArchiveXL SHARED ${ARCHIVEXL_SOURCES})
if(MAC_LAYOUT_GUARD)
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/MacLayoutGuard.cpp")
endif()

set_target_properties(ArchiveXL PROPERTIES
    OUTPUT_NAME "ArchiveXL"
    PREFIX ""
    SUFFIX ".dylib"
)

target_include_directories(ArchiveXL PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${SDK_OVERLAY}"
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    "${AXL_DIR}/lib"
    "${AXL_DIR}/src"
    "${SDK_INCLUDE}"
    "${AXL_DIR}/vendor/nameof/include"
    "${AXL_DIR}/vendor/semver/include"
)

target_link_libraries(ArchiveXL PRIVATE
    spdlog::spdlog
    fmt::fmt
    yaml-cpp::yaml-cpp
    "-framework Foundation"
)

target_precompile_headers(ArchiveXL PRIVATE "${AXL_DIR}/src/pch.hpp")

target_compile_options(ArchiveXL PRIVATE
    -fno-rtti
    -fexceptions
    -Wno-deprecated-declarations
    -fvisibility=hidden
    -fvisibility-inlines-hidden
)

# The plugin folder carries the vendored spdlog, fmt and yaml-cpp next to ArchiveXL.dylib. Rewrite the Homebrew install
# names to @loader_path and re-sign (install_name_tool invalidates the signature).
add_custom_command(TARGET ArchiveXL POST_BUILD
    COMMAND /bin/bash "${CMAKE_CURRENT_SOURCE_DIR}/fix-install-names.sh" "$<TARGET_FILE:ArchiveXL>"
    VERBATIM
)

# Fail the build when an ArchiveXL function is declared but not compiled in (the link flag above would hide it).
add_custom_command(TARGET ArchiveXL POST_BUILD
    COMMAND /bin/bash "${CMAKE_CURRENT_SOURCE_DIR}/check-undefined.sh" "$<TARGET_FILE:ArchiveXL>"
    VERBATIM
)

message(STATUS "ArchiveXL ${PROJECT_VERSION} configured for macOS")
