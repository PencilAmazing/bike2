cmake_minimum_required(VERSION 3.24)
project (bike LANGUAGES CXX C )

set(SRC "src/main.cpp" )
set(HDR "")

if(UNIX)
    set(raylib_location "$ENV{HOME}/Dev/raylib")
else()
    set(raylib_location "D:/dev/raylib")
endif()

add_subdirectory(${raylib_location} ${CMAKE_CURRENT_BINARY_DIR}/raylib)

add_executable(bike ${SRC} "src/draw.h")
target_include_directories(bike PRIVATE .)

target_link_libraries(bike raylib)

set (assetSource "${CMAKE_SOURCE_DIR}/asset")
set (assetDest "${CMAKE_CURRENT_BINARY_DIR}/asset")
if (UNIX)
add_custom_command(
    TARGET bike POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E create_symlink ${assetSource} ${assetDest}
    DEPENDS ${assetDest}
    COMMENT "symbolic link resources folder from ${assetSource} => ${assetDest}")
else()
    file(TO_NATIVE_PATH "${assetDest}" _dstDir)
    file(TO_NATIVE_PATH "${assetSource}" _srcDir)
#    execute_process(COMMAND cmd.exe /c mklink /J "${_dstDir}" "${_srcDir}")
    #message("Created junction link to asset folder from ${assetSource} to ${assetDest}")
endif()
# https://stackoverflow.com/a/61244115/11367507