# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

set(prefix "${TEST_PREFIX}")
set(suffix "${TEST_SUFFIX}")
set(spec ${TEST_SPEC})
set(extra_args ${TEST_EXTRA_ARGS})
set(properties ${TEST_PROPERTIES})
set(script)
set(suite)
set(tests)

function(add_command NAME)
  set(_args "")
  foreach(_arg ${ARGN})
    if(_arg MATCHES "[^-./:a-zA-Z0-9_]")
      set(_args "${_args} [==[${_arg}]==]") # form a bracket_argument
    else()
      set(_args "${_args} ${_arg}")
    endif()
  endforeach()
  set(script "${script}${NAME}(${_args})\n" PARENT_SCOPE)
endfunction()

# Run test executable to get list of available tests
if(NOT EXISTS "${TEST_EXECUTABLE}")
  message(FATAL_ERROR
    "Specified test executable '${TEST_EXECUTABLE}' does not exist"
  )
endif()
execute_process(
  COMMAND ${TEST_EXECUTOR} "${TEST_EXECUTABLE}" ${spec} --list-test-names-only
  OUTPUT_VARIABLE output
  RESULT_VARIABLE result
)
# Catch --list-test-names-only reports the number of tests, so 0 is... surprising
if(${result} EQUAL 0)
  message(WARNING
    "Test executable '${TEST_EXECUTABLE}' contains no tests!\n"
  )
elseif(${result} LESS 0)
  message(FATAL_ERROR
    "Error running test executable '${TEST_EXECUTABLE}':\n"
    "  Result: ${result}\n"
    "  Output: ${output}\n"
  )
endif()

string(REPLACE "\n" ";" output "${output}")

# Parse output
foreach(line ${output})
  set(test ${line})
  # Escape characters in test case names that would be parsed by Catch2
  set(test_name ${test})
  foreach(char , [ ])
    string(REPLACE ${char} "\\${char}" test_name ${test_name})
  endforeach(char)
  # ...and add to script
  add_command(add_test
    "${prefix}${test}${suffix}"
    ${TEST_EXECUTOR}
    "${TEST_EXECUTABLE}"
    "${test_name}"
    ${extra_args}
  )
  add_command(set_tests_properties
    "${prefix}${test}${suffix}"
    PROPERTIES
    WORKING_DIRECTORY "${TEST_WORKING_DIR}"
    ${properties}
  )
  list(APPEND tests "${prefix}${test}${suffix}")
endforeach()

# Create a list of all discovered tests, which users may use to e.g. set
# properties on the tests
add_command(set ${TEST_LIST} ${tests})

# Write CTest script
file(WRITE "${CTEST_FILE}" "${script}")
