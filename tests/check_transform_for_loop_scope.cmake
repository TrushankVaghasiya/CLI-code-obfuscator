
execute_process(
    COMMAND "${OBFUSCATOR}"
        -i "${INPUT}"
        -o "${OUTPUT}"
        --level 1
    RESULT_VARIABLE result
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "Obfuscator execution failed")
endif()

file(READ "${OUTPUT}" content)

# The loop variable declaration must be renamed.
if(NOT content MATCHES "for \\(int obf_var_[0-9]+ = 0;")
    message(FATAL_ERROR "For-loop variable declaration was not renamed")
endif()

# The variable declared after the loop must remain unchanged.
# The variable declared after the loop must also be renamed.
if(NOT content MATCHES "int obf_var_[0-9]+ = 100;")
    message(FATAL_ERROR "Variable outside the loop was not renamed")
endif()

# Its reference must also be renamed.
if(NOT content MATCHES "cout << obf_var_[0-9]+ << '\\\\n';")
    message(FATAL_ERROR "Reference outside the loop was not renamed")
endif()

message(STATUS "For-loop scope test passed")
