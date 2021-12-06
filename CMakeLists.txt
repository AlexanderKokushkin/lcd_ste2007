#idf_component_register(SRCS "lcd_ste2007.cpp" INCLUDE_DIRS "." REQUIRES generic_constants)
idf_component_register(INCLUDE_DIRS ".")

#set_source_files_properties( lcd_ste2007.cpp
#    PROPERTIES COMPILE_FLAGS
#    "-fpermissive -Wno-unused-variable -std=gnu++17"
#)
