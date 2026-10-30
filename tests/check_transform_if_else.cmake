
if(NOT DEFINED OBFUSCATOR OR
   NOT DEFINED INPUT OR
   NOT DEFINED OUTPUT)
    message(FATAL_ERROR
        "OBFUSCATOR, INPUT, and OUTPUT are required")
endif()

execute_process(
    COMMAND "${OBFUSCATOR}"
        -i "${INPUT}"
        -o "${OUTPUT}"
        --level 1
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "Obfuscator failed.\nSTDOUT:\n${stdout}\nSTDERR:\n${stderr}")
endif()

if(NOT EXISTS "${OUTPUT}")
    message(FATAL_ERROR "Output file was not created")
endif()

file(READ "${OUTPUT}" transformed)

# Check declaration and condition.
if(NOT transformed MATCHES "int obf_var_1 = 10")
    message(FATAL_ERROR "If initializer declaration was not renamed")
endif()

if(NOT transformed MATCHES "obf_var_1 > 5")
    message(FATAL_ERROR "If condition reference was not renamed")
endif()

# Check both branch references.
string(REGEX MATCHALL
    "std::cout << obf_var_1"
    references
    "${transformed}")

list(LENGTH references reference_count)

if(NOT reference_count EQUAL 2)
    message(FATAL_ERROR
        "Expected 2 renamed branch references, found ${reference_count}")
endif()

message(STATUS "If-else test passed")
