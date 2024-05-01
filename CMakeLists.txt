cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

function(vi_macro_show M)
	if(DEFINED ${M})
		message(STATUS "${M}: \'${${M}}\'")
	else()
		message(STATUS "${M}: <not defined>")
	endif()
endfunction()

set(CMAKE_CONFIGURATION_TYPES Debug Release)

project(vi_metrics_time LANGUAGES C CXX ASM_MASM)

set(VI_OUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(VI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)

set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
set(Boost_USE_DEBUG_LIBS        OFF)  # ignore debug libs and
#set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
set(Boost_USE_MULTITHREADED      ON)
#set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.75 COMPONENTS "timer")

### Files #####################################################################
set(FILE_GROUP
  "${VI_SOURCE_DIR}/$<$<PLATFORM_ID:Linux>:tm_linux.cpp>"
  "${VI_SOURCE_DIR}/$<$<PLATFORM_ID:Windows>:tm_wintel.cpp>"
  "${VI_SOURCE_DIR}/header.h"
  "${VI_SOURCE_DIR}/main.cpp"
  "${VI_SOURCE_DIR}/misc.cpp"
  "${VI_SOURCE_DIR}/misc.h"
  "${VI_SOURCE_DIR}/tm_asm.cpp"
  "${VI_SOURCE_DIR}/tm_standard.cpp"
)

if(Boost_FOUND)
    list(APPEND FILE_GROUP "${VI_SOURCE_DIR}/tm_boost.cpp")
endif()

if(DEFINED WIN32)
    list(APPEND FILE_GROUP "${VI_SOURCE_DIR}/tm_masm.asm")
endif()

source_group("Source Files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if(Boost_FOUND)
vi_macro_show(Boost_INCLUDE_DIRS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
vi_macro_show(Boost_LIBRARY_DIRS)
    target_link_directories(${PROJECT_NAME} PRIVATE ${Boost_LIBRARY_DIRS})
endif()

### Compiler ##################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  C_STANDARD 11
  CXX_EXTENSIONS OFF
  CXX_STANDARD_REQUIRED ON
  OUTPUT_NAME_RELEASE "${PROJECT_NAME}"
  OUTPUT_NAME_DEBUG   "${PROJECT_NAME}_d"

  LIBRARY_OUTPUT_DIRECTORY	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY	"${VI_OUT_DIR}"

  LIBRARY_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  LIBRARY_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
)

target_compile_definitions(${PROJECT_NAME}
PRIVATE
  $<$<CONFIG:Release>:NDEBUG>
)

if (WIN32)
  target_compile_options(${PROJECT_NAME}
  PRIVATE
    /MP /W3 /nologo /EHsc /Zi
    $<$<CONFIG:Release>: /MT  /O2 /Oi /GL /Gy>
    $<$<CONFIG:Debug>:   /MTd /Od /RTC1>
  )
elseif (UNIX)
  target_compile_options(${PROJECT_NAME}
  PRIVATE
    -Wno-psabi #suppress "note: parameter passing for argument of type <...> changed in GCC 7.1" message.
    $<$<CONFIG:Release>:-O3 -ggdb3>
  )
endif()

### Linker ####################################################################
if (WIN32)
  target_link_options(${PROJECT_NAME}
  PRIVATE
    /SUBSYSTEM:CONSOLE /DEBUG
    $<$<CONFIG:Release>: /INCREMENTAL:NO /LTCG /OPT:REF /OPT:ICF>
    $<$<CONFIG:Debug>:   /INCREMENTAL /DEBUG>
  )
  target_link_libraries(${PROJECT_NAME}
  PRIVATE
    Winmm # For timeGetSystemTime and timeGetTime
    Ntdll # For NtQueryTimerResolution and NtQuerySystemTime
    Mincore # For QueryInterruptTimePrecise and QueryInterruptTime
  )
elseif (UNIX)
  target_link_libraries(${PROJECT_NAME}
  PRIVATE
    pthread
#    stdc++fs
  )
endif()
