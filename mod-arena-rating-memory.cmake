#
# Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
#
# Included inline by modules/CMakeLists.txt. Registers the module's tests with the core's
# unit_tests target, which picks them up from these global properties.
#

if (BUILD_TESTING)
  file(GLOB_RECURSE MODULE_TEST_SOURCES
    "${CMAKE_SOURCE_DIR}/modules/mod-arena-rating-memory/tests/*.cpp")

  if(MODULE_TEST_SOURCES)
    set_property(GLOBAL APPEND PROPERTY ACORE_MODULE_TEST_SOURCES ${MODULE_TEST_SOURCES})

    set_property(GLOBAL APPEND PROPERTY ACORE_MODULE_TEST_INCLUDES
      "${CMAKE_SOURCE_DIR}/modules/mod-arena-rating-memory/src")

    list(LENGTH MODULE_TEST_SOURCES TEST_FILE_COUNT)
    message(STATUS "  +- Registered ${TEST_FILE_COUNT} test file(s) from mod-arena-rating-memory")
  endif()
endif()
