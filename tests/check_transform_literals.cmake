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
        --level 2
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "Obfuscator failed.\nSTDOUT:\n${stdout}\nSTDERR:\n${stderr}")
endif()

if(NOT EXISTS "${OUTPUT}")
    message(FATAL_ERROR "Transformed output was not created")
endif()

file(READ "${OUTPUT}" transformed)

# Character literal must remain unchanged.
string(FIND "${transformed}" "'A'" char_pos)

if(char_pos EQUAL -1)
    message(FATAL_ERROR "Character literal was modified")
endif()

# Comment-like text inside the string must remain unchanged.
string(FIND "${transformed}"
    "Hello // world /* not a comment */"
    string_pos)

if(string_pos EQUAL -1)
    message(FATAL_ERROR
        "String literal was modified during comment removal")
endif()

# The variable should still be renamed at Level 2.
string(FIND "${transformed}" "obf_var_1" renamed_pos)

if(renamed_pos EQUAL -1)
    message(FATAL_ERROR
        "Expected variable renaming at Level 2")
endif()

message(STATUS
    "String and character literal preservation test passed")