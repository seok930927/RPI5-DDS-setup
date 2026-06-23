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

# 자동 시나리오 러너 (RPI4=auto_pub 오케스트레이터, RPI5=auto_sub 팔로워)
add_executable(auto_pub auto_pub.cpp ${TYPE_SOURCES})
target_link_libraries(auto_pub fastdds fastcdr)

add_executable(auto_sub auto_sub.cpp ${TYPE_SOURCES})
target_link_libraries(auto_sub fastdds fastcdr)
