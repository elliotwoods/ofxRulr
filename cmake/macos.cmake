# Build the existing oF library with its supplied Xcode project before configuring.
if(NOT APPLE)
    message(FATAL_ERROR "This build currently supports macOS only")
endif()
get_filename_component(OF_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
find_package(OpenCV 4.12 REQUIRED)
if(NOT OpenCV_VERSION VERSION_LESS 5)
    message(FATAL_ERROR "Use OpenCV 4.12 and set OpenCV_DIR to its lib/cmake/opencv4 directory")
endif()
find_package(Ceres REQUIRED)
find_package(NLopt REQUIRED)

add_library(RulrDependencies INTERFACE)
file(GLOB OF_HEADER_DIRS LIST_DIRECTORIES true "${OF_ROOT}/libs/openFrameworks/*")
file(GLOB OF_LIBRARY_HEADERS LIST_DIRECTORIES true "${OF_ROOT}/libs/*/include")
target_include_directories(RulrDependencies INTERFACE
    "${OF_ROOT}/libs/openFrameworks" ${OF_HEADER_DIRS} ${OF_LIBRARY_HEADERS}
    "${OF_ROOT}/libs/freetype/include/freetype2" ${OpenCV_INCLUDE_DIRS})
target_compile_definitions(RulrDependencies INTERFACE
    GL_SILENCE_DEPRECATION GLM_FORCE_CTOR_INIT GLM_ENABLE_EXPERIMENTAL
    HAS_OFXCVGUI HAS_OFXSINGLETON HAS_OFXGRABCAM HAS_OFXAUDIODECODER GLM_FORCE_UNRESTRICTED_GENTYPE GLM_FORCE_XYZW_ONLY DISABLE_OFXWEBWIDGETS)
file(GLOB OF_LIBRARIES "${OF_ROOT}/libs/*/lib/macos/*.xcframework/macos-*/*.a")
target_link_libraries(RulrDependencies INTERFACE
    "${OF_ROOT}/libs/openFrameworksCompiled/lib/osx/openFrameworks.a"
    ${OF_LIBRARIES} ${OpenCV_LIBS} Ceres::ceres NLopt::nlopt)
foreach(FRAMEWORK Accelerate AppKit ApplicationServices AudioToolbox AVFoundation
    Cocoa CoreAudio CoreFoundation CoreMedia CoreServices CoreVideo Foundation
    IOKit OpenGL QuartzCore Security SystemConfiguration Metal)
    target_link_libraries(RulrDependencies INTERFACE "-framework ${FRAMEWORK}")
endforeach()

set(RULR_ADDONS ofxAssets ofxAudioDecoder ofxClipboard ofxCvGui ofxCvMin
    ofxGrabCam ofxLiquidEvent ofxMachineVision ofxNonLinearFit ofxPlugin ofxRay
    ofxSingleton ofxSpinCursor ofxTriangulate ofxTriangle ofxGraycode ofxCeres
    ofxOsc ofxAssimpModelLoader ofxObjLoader ofxTextInputField)
set(RULR_SOURCES)
foreach(ADDON IN LISTS RULR_ADDONS)
    set(ADDON_ROOT "${OF_ROOT}/addons/${ADDON}")
    if(NOT EXISTS "${ADDON_ROOT}/src")
        message(FATAL_ERROR "Missing addon: ${ADDON}")
    endif()
    target_include_directories(RulrDependencies INTERFACE "${ADDON_ROOT}/src")
    file(GLOB_RECURSE ADDON_SOURCES CONFIGURE_DEPENDS
        "${ADDON_ROOT}/src/*.cpp" "${ADDON_ROOT}/src/*.mm" "${ADDON_ROOT}/src/*.c")
    list(FILTER ADDON_SOURCES EXCLUDE REGEX "/pch_[^/]*\\.cpp$")
    list(APPEND RULR_SOURCES ${ADDON_SOURCES})
endforeach()
foreach(PART Core Nodes)
    file(GLOB_RECURSE PART_SOURCES CONFIGURE_DEPENDS "${OF_ROOT}/addons/ofxRulr/${PART}/src/ofxRulr/*.cpp")
    list(APPEND RULR_SOURCES ${PART_SOURCES})
    target_include_directories(RulrDependencies INTERFACE "${OF_ROOT}/addons/ofxRulr/${PART}/src")
endforeach()
target_include_directories(RulrDependencies INTERFACE
    "${OF_ROOT}/addons/ofxRulr/Core/3rdparty"
    "${OF_ROOT}/addons/ofxTriangle/libs/Triangle/include"
    "${OF_ROOT}/addons/ofxAudioDecoder/libs/libaudiodecoder/include"
    "${OF_ROOT}/addons/ofxOsc/libs/oscpack/src"
    "${OF_ROOT}/addons/ofxOsc/libs/oscpack/src/osc"
    "${OF_ROOT}/addons/ofxOsc/libs/oscpack/src/ip"
    "${OF_ROOT}/addons/ofxObjLoader/libs")
file(GLOB_RECURSE OSC_SOURCES "${OF_ROOT}/addons/ofxOsc/libs/oscpack/src/*.cpp")
list(FILTER OSC_SOURCES EXCLUDE REGEX "/win32/")
file(GLOB TRIANGLE_SOURCES "${OF_ROOT}/addons/ofxTriangle/libs/Triangle/src/*.cpp")
list(APPEND RULR_SOURCES ${OSC_SOURCES} ${TRIANGLE_SOURCES}
    "${OF_ROOT}/addons/ofxObjLoader/libs/glm.c"
    "${OF_ROOT}/addons/ofxAudioDecoder/libs/libaudiodecoder/src/audiodecoderbase.cpp"
    "${OF_ROOT}/addons/ofxAudioDecoder/libs/libaudiodecoder/src/audiodecodercoreaudio.cpp")
file(GLOB ASSIMP_LIBRARIES "${OF_ROOT}/addons/ofxAssimpModelLoader/libs/assimp/lib/macos/*.xcframework/macos-*/*.a")
file(GLOB ASSIMP_HEADERS "${OF_ROOT}/addons/ofxAssimpModelLoader/libs/assimp/lib/macos/*.xcframework/macos-*/Headers")
target_include_directories(RulrDependencies INTERFACE ${ASSIMP_HEADERS})
target_link_libraries(RulrDependencies INTERFACE ${ASSIMP_LIBRARIES})
add_library(RulrLibrary SHARED ${RULR_SOURCES})
target_link_libraries(RulrLibrary PUBLIC RulrDependencies)
target_precompile_headers(RulrLibrary PRIVATE "${OF_ROOT}/addons/ofxRulr/Nodes/src/pch_RulrNodes.h")
set_target_properties(RulrLibrary PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

set_source_files_properties("${OF_ROOT}/addons/ofxObjLoader/libs/glm.c" PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

set_source_files_properties(${OSC_SOURCES} ${TRIANGLE_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

function(rulr_add_plugin NAME)
    set(PLUGIN_ROOT "${OF_ROOT}/addons/ofxRulr/${NAME}")
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${PLUGIN_ROOT}/src/*.cpp")
    add_library(${NAME} SHARED ${SOURCES})
    target_include_directories(${NAME} PRIVATE "${PLUGIN_ROOT}/src")
    target_link_libraries(${NAME} PRIVATE RulrLibrary)
    target_precompile_headers(${NAME} PRIVATE "${PLUGIN_ROOT}/src/pch_${NAME}.h")
    set_target_properties(${NAME} PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
endfunction()
