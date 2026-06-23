
#
# Common compile flags
#

set(DRIVER_COMPILE_FLAGS
    -Werror
    -Wall
    -fPIE
    -fno-stack-protector
    -fno-builtin
    -ffreestanding
    -fshort-wchar
    -mno-red-zone
)

#
# Common link flags
#

set(DRIVER_LINK_FLAGS
    -nostdlib
    -pie
    -Wl,-e,DriverMain
)

#
# Common include directories
#

set(DRIVER_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/lib/driver_runtime/include
    ${CMAKE_SOURCE_DIR}/headers
)

#
# Common library directories
#

link_directories(
    ${CMAKE_SOURCE_DIR}/build
    ${CMAKE_SOURCE_DIR}/lib/driver_runtime
)

#
# Common libraries
#

set(DRIVER_LIBRARIES
    KNeOS
    NeOS.sys
)

#
# Automatically build every subdirectory as a driver
#

file(GLOB DRIVER_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} */)
set(ALL_DRIVER_TARGETS "")

foreach(DRIVER_DIR ${DRIVER_DIRS})

    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${DRIVER_DIR}")
        #
        # Driver name
        #

        string(REGEX REPLACE "/$" "" DRIVER_NAME ${DRIVER_DIR})

        #
        # Collect sources
        #

        file(GLOB DRIVER_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/${DRIVER_DIR}/*.c"
        )

        #
        # Skip empty directories
        #

        if(DRIVER_SOURCES)

            #
            # Build shared object with custom extension
            #

            list(APPEND ALL_DRIVER_TARGETS ${DRIVER_NAME})
            add_executable(${DRIVER_NAME} ${DRIVER_SOURCES})
            add_dependencies(${DRIVER_NAME} ${KERNEL_EXE})

            #
            # Output extension: .drv
            #

            set_target_properties(${DRIVER_NAME} PROPERTIES
                SUFFIX ".drv"
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${DRIVER_DIR}
            )

            #
            # Includes
            #

            target_include_directories(${DRIVER_NAME} PRIVATE
                ${DRIVER_INCLUDE_DIRS}
            )

            #
            # Compile flags
            #

            target_compile_options(${DRIVER_NAME} PRIVATE
                ${DRIVER_COMPILE_FLAGS}
            )

            #
            # Link flags
            #

            target_link_options(${DRIVER_NAME} PRIVATE
                ${DRIVER_LINK_FLAGS}
            )

            #
            # Libraries
            #

            target_link_libraries(${DRIVER_NAME} PRIVATE
                ${DRIVER_LIBRARIES}
            )

            message(STATUS "Configured driver: ${DRIVER_NAME}")

        endif()

    endif()

endforeach()

set(ALL_DRIVER_TARGETS ${ALL_DRIVER_TARGETS} PARENT_SCOPE)
