set(TARGET webapi)
add_executable(${TARGET} webapi.cpp third-party/cpp-httplib/httplib.h)
target_link_libraries(${TARGET} PRIVATE llama ${CMAKE_THREAD_LIBS_INIT})
target_compile_features(${TARGET} PRIVATE cxx_std_11)

add_subdirectory(third-party/llama.cpp)

