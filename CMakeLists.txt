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
    stdinclude.hpp
    json/json.hpp
    )
    
include_directories(extlib/cpr/include)

add_library(${PROJECT_NAME} ${FACECHAT_SOURCE} ${FACECHAT_INCLUDE})

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(PUBLIC ${PROJECT_NAME} "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(${PROJECT_NAME} "${CMAKE_THREAD_LIBS_INIT}")
endif()

target_link_libraries(
    ${PROJECT_NAME}
    cpr
)
