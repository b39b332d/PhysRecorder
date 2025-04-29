# copy from https://gist.github.com/end2endzone/1131ebeb446d4ffa46f562e6a2f5544f
# Inspired from https://stackoverflow.com/questions/32183975/how-to-print-all-the-properties-of-a-target-in-cmake

# Get all propreties that cmake supports
execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)

# Convert command output into a CMake list
STRING(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
STRING(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")

# Prints the given property of the given target.
# Recursive for properties ending with <CONFIG>
# Skips properties that are not set.
function(print_target_property tgt prop)
  string(FIND ${prop} "<CONFIG>" isconfig)
  if (NOT ${isconfig} MATCHES -1)
    # this is a <CONFIG> property

    if (${CMAKE_BUILD_TYPE})
      # Expecting makefile generator

      #try ${CMAKE_BUILD_TYPE}
      string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" propconfig ${prop})
      print_target_property(${tgt} ${propconfig})
    else()
      # Expecting Visual Studio generator

      #try RELEASE
      string(REPLACE "<CONFIG>" "RELEASE" propconfig ${prop})
      print_target_property(${tgt} ${propconfig})

      #try DEBUG
      string(REPLACE "<CONFIG>" "DEBUG" propconfig ${prop})
      print_target_property(${tgt} ${propconfig})
    endif()
  else()
    # message ("Checking ${prop}")
    get_property(propval TARGET ${tgt} PROPERTY ${prop} SET)
    if (propval)
        get_target_property(propval ${tgt} ${prop})
        message ("${tgt}.${prop}='${propval}'")
    endif()
  endif()
endfunction(print_target_property)

# Prints all CMake properties of the given target.
function(print_target_properties tgt)
    if(NOT TARGET ${tgt})
      message(There is no target named '${tgt}')
      return()
    endif()

    foreach (prop ${CMAKE_PROPERTY_LIST})
        # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
        if(prop STREQUAL "LOCATION" OR prop MATCHES "^LOCATION_" OR prop MATCHES "_LOCATION$")
            continue()
        endif()

        # Try to print this property.
        print_target_property(${tgt} ${prop})
    endforeach(prop)
endfunction(print_target_properties)