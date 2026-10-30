
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

# Check that the for-loop declaration was renamed.
if(NOT content MATCHES "for \\(int obf_var_[0-9]+ = 0;")
    message(FATAL_ERROR "For-loop variable declaration was not renamed")
endif()

# Check that the original variable identifier does not remain.
if(content MATCHES "(^|[^a-zA-Z0-9_])i([^a-zA-Z0-9_]|$)")
    message(FATAL_ERROR "Original loop variable name still exists")
endif()

message(STATUS "For-loop transformation test passed")
