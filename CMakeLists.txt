cmake_minimum_required(VERSION 2.8)
file(GLOB THIS_APP_SOURCES *.cpp)
file(GLOB THIS_APP_HEADERS *.h)

project(realtime_pthread)
add_executable(${PROJECT_NAME} ${THIS_APP_SOURCES} ${THIS_APP_HEADERS})
target_link_libraries(${PROJECT_NAME} pthread rt)

