cmake_minimum_required(VERSION 3.16)
project(dds_test CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fastcdr REQUIRED)
find_package(fastdds REQUIRED)

set(TYPE_SOURCES
    HelloWorldPubSubTypes.cxx
    HelloWorldTypeObjectSupport.cxx)

add_executable(publisher publisher.cpp ${TYPE_SOURCES})
target_link_libraries(publisher fastdds fastcdr)

add_executable(subscriber subscriber.cpp ${TYPE_SOURCES})
target_link_libraries(subscriber fastdds fastcdr)
