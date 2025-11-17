# cmake files support debug production
include("${CMAKE_CURRENT_LIST_DIR}/rule.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/file.cmake")

set(rec_001_default_library_list )

# Handle files with suffix (s|as|asm|AS|ASM|As|aS|Asm), for group default-XC8
if(rec_001_default_default_XC8_FILE_TYPE_assemble)
add_library(rec_001_default_default_XC8_assemble OBJECT ${rec_001_default_default_XC8_FILE_TYPE_assemble})
    rec_001_default_default_XC8_assemble_rule(rec_001_default_default_XC8_assemble)
    list(APPEND rec_001_default_library_list "$<TARGET_OBJECTS:rec_001_default_default_XC8_assemble>")
endif()

# Handle files with suffix S, for group default-XC8
if(rec_001_default_default_XC8_FILE_TYPE_assemblePreprocess)
add_library(rec_001_default_default_XC8_assemblePreprocess OBJECT ${rec_001_default_default_XC8_FILE_TYPE_assemblePreprocess})
    rec_001_default_default_XC8_assemblePreprocess_rule(rec_001_default_default_XC8_assemblePreprocess)
    list(APPEND rec_001_default_library_list "$<TARGET_OBJECTS:rec_001_default_default_XC8_assemblePreprocess>")
endif()

# Handle files with suffix [cC], for group default-XC8
if(rec_001_default_default_XC8_FILE_TYPE_compile)
add_library(rec_001_default_default_XC8_compile OBJECT ${rec_001_default_default_XC8_FILE_TYPE_compile})
    rec_001_default_default_XC8_compile_rule(rec_001_default_default_XC8_compile)
    list(APPEND rec_001_default_library_list "$<TARGET_OBJECTS:rec_001_default_default_XC8_compile>")
endif()

add_executable(rec_001_default_image_tNjcvb4u ${rec_001_default_library_list})

set_target_properties(rec_001_default_image_tNjcvb4u PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${rec_001_default_output_dir})
set_target_properties(rec_001_default_image_tNjcvb4u PROPERTIES OUTPUT_NAME "default")
set_target_properties(rec_001_default_image_tNjcvb4u PROPERTIES SUFFIX ".elf")

target_link_libraries(rec_001_default_image_tNjcvb4u PRIVATE ${rec_001_default_default_XC8_FILE_TYPE_link})


# Add the link options from the rule file.
rec_001_default_link_rule(rec_001_default_image_tNjcvb4u)



