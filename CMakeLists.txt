cmake_minimum_required(VERSION 3.10)
project(image2ascii)

set (SOURCE_FILES
	src/script.c
	stb_image/stb_image.c
)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	add_compile_options(-Wall -Werror -pedantic)
endif()


add_executable(image2ascii ${SOURCE_FILES})

if (UNIX)
	target_link_libraries(image2ascii m)
endif()

