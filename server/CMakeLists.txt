set(TARGET webapi)

add_compile_options($<$<COMPILE_LANGUAGE:C>:/arch:AVX2>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/arch:AVX2>)
add_compile_options(-mavx2)

add_executable(${TARGET} webapi.cpp llamalib.cpp llamalib.h webutils.cpp webutils.h third-party/cpp-httplib/httplib.h)
target_link_libraries(${TARGET} PRIVATE common llama ${CMAKE_THREAD_LIBS_INIT})
target_compile_features(${TARGET} PRIVATE cxx_std_11)


add_subdirectory(third-party/llama.cpp)
add_subdirectory(third-party/llama.cpp/examples)

