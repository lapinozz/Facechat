cmake_minimum_required(VERSION 2.8)

add_subdirectory(extlib/cpr)

project("Facechat")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "(GNU|.*Clang)")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

set(FACECHAT_SOURCE
    Facechat.cpp
    Utility.cpp
    )

set(FACECHAT_INCLUDE
    Facechat.h
    Utility.hpp
    json/json.hpp
    stdinclude.hpp
    )

include_directories(extlib/cpr/include)

add_executable(${PROJECT_NAME} ${FACECHAT_SOURCE} ${FACECHAT_INCLUDE})

target_link_libraries(
    ${PROJECT_NAME}
    libcpr
)
