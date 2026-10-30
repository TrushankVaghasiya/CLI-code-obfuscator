
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

# Check declaration.
if(NOT transformed MATCHES "int obf_var_1 = 10")
    message(FATAL_ERROR "If initializer declaration was not renamed")
endif()

# Check condition reference.
if(NOT transformed MATCHES "obf_var_1 > 5")
    message(FATAL_ERROR "If condition reference was not renamed")
endif()

# Check body reference.
if(NOT transformed MATCHES "std::cout << obf_var_1")
    message(FATAL_ERROR "If body reference was not renamed")
endif()

message(STATUS "If condition test passed")
