cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

function(vi_macro_show M)
	if(DEFINED ${M})
		message("${M}: \'${${M}}\'")
	else()
		message("${M}: <not defined>")
	endif()
endfunction()

#vi_macro_show(MSVC_VERSION)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

project(vi_metrics_time LANGUAGES C CXX ASM_MASM)

set(VI_OUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(VI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
#vi_macro_show(VI_OUT_DIR)
#vi_macro_show(VI_SOURCE_DIR)

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

if(DEFINED WIN32)
    list(APPEND FILE_GROUP "${VI_SOURCE_DIR}/tm_masm.asm")
endif()

source_group("Source Files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

#set_property(SOURCE "${VI_SOURCE_DIR}/tm_masm.asm" PROPERTY LANGUAGE ASM)

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
