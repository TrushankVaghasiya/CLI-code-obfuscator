
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

if(NOT EXISTS "${OUTPUT}")
    message(FATAL_ERROR "Output file was not generated")
endif()

file(READ "${OUTPUT}" transformed)

# Check outer variable declaration.
if(NOT transformed MATCHES "int obf_var_1 = 10;")
    message(FATAL_ERROR "Outer variable was not renamed correctly")
endif()

# Check lambda parameter declaration.
if(NOT transformed MATCHES "int obf_var_2")
    message(FATAL_ERROR "Lambda parameter was not renamed correctly")
endif()

# Check lambda parameter reference.
if(NOT transformed MATCHES "cout << obf_var_2")
    message(FATAL_ERROR "Lambda parameter reference was not renamed")
endif()

# Check outer variable reference.
if(NOT transformed MATCHES "cout << obf_var_1")
    message(FATAL_ERROR "Outer variable reference was not renamed")
endif()

# Check original identifier is completely removed.
if(transformed MATCHES "(^|[^A-Za-z0-9_])value([^A-Za-z0-9_]|$)")
    message(FATAL_ERROR "Original identifier 'value' still exists")
endif()

message(STATUS "Lambda parameter validation passed")
