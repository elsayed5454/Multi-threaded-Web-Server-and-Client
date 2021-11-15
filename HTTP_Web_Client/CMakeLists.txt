cmake_minimum_required(VERSION 3.20)
project(HTTP_Web_Client)

set(CMAKE_CXX_STANDARD 17)

add_executable(HTTP_Web_Client main.cpp HTTP_Request.cpp HTTP_Request.h)
