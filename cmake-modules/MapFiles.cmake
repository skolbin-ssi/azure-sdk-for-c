
add_link_options(/MAP)

# if(MSVC)
#   add_link_options(/MAP)
# elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
#   # add_link_options(-map)
# else()
#   add_link_options(-Wl,-Map)
# endif()