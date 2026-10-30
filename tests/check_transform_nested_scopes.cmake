
if(NOT DEFINED OBFUSCATOR OR NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "Missing required test arguments")
endif()

execute_process(
    COMMAND "${OBFUSCATOR}"
            --input "${INPUT}"
            --output "${OUTPUT}"
            --level 1
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "Obfuscator execution failed:\n${stdout}\n${stderr}")
endif()

file(READ "${OUTPUT}" transformed)

# Check outer variable declaration.
if(NOT transformed MATCHES "int obf_var_1 = 10")
    message(FATAL_ERROR "Outer variable was not renamed correctly")
endif()

# Check if-condition declaration.
if(NOT transformed MATCHES "if \\(int obf_var_2 = 20\\)")
    message(FATAL_ERROR "If condition variable was not renamed correctly")
endif()

# Check while-condition declaration.
if(NOT transformed MATCHES "while \\(int obf_var_3 = 30\\)")
    message(FATAL_ERROR "While condition variable was not renamed correctly")
endif()

# Check references inside the if body.
if(NOT transformed MATCHES "cout << obf_var_2")
    message(FATAL_ERROR "If variable reference is incorrect")
endif()

# Check references inside the while body.
if(NOT transformed MATCHES "cout << obf_var_3")
    message(FATAL_ERROR "While variable reference is incorrect")
endif()

# Check reference to the outer variable after the if statement.
if(NOT transformed MATCHES "cout << obf_var_1")
    message(FATAL_ERROR "Outer variable reference is incorrect")
endif()

# Ensure original variable name is completely removed.
if(transformed MATCHES "\\bvalue\\b")
    message(FATAL_ERROR "Original variable name still exists")
endif()

message(STATUS "Nested scopes transformation test passed")
