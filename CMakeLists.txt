cmake_minimum_required (VERSION 3.0)

project (easyb)

# set default build type if not specified by user
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE debug)
endif()

if(ARM_BOARD)
	message("build for arm board...")
	set(CMAKE_C_COMPILER "/home/easyboard/corelinux/v3ssdk/buildroot/out/host/bin/arm-buildroot-linux-gnueabihf-gcc")
	set(CMAKE_CXX_COMPILER "/home/easyboard/corelinux/v3ssdk/buildroot/out/host/bin/arm-buildroot-linux-gnueabihf-g++")
	
	set(bsp_src ${CMAKE_CURRENT_SOURCE_DIR}/src/bsp_arm/app.c)
else()
	message("build for fake...")
	set(CMAKE_C_COMPILER "/Library/Developer/CommandLineTools/usr/bin/gcc")
	set(CMAKE_CXX_COMPILER "/Library/Developer/CommandLineTools/usr/bin/g++")
	
	set(bsp_src ${CMAKE_CURRENT_SOURCE_DIR}/src/bsp_fake/app.c)
endif()

set(CMAKE_C_FLAGS         "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG   "-g -O0 -DDEBUG")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include 
/usr/local/include
/home/easyboard/corelinux/v3ssdk/app/include
)

link_directories(${easyb_BINARY_DIR} 
/usr/local/lib
/home/easyboard/corelinux/v3ssdk/buildroot/out/target/usr/lib
)

set(easy_headers ${CMAKE_CURRENT_SOURCE_DIR}/include/easy_board.h
)

set(easy_srcs ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
${CMAKE_CURRENT_SOURCE_DIR}/src/rpa_queue.c
${CMAKE_CURRENT_SOURCE_DIR}/src/msg_queue.c
${bsp_src}
) 

add_executable(easyb ${easy_srcs})
target_link_libraries(easyb mdm cjson paho-mqtt3c pthread)


