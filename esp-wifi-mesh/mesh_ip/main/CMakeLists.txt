idf_component_register(SRCS "mesh_ip.c"
                            "mesh_netif.c"
                    INCLUDE_DIRS "." "include")
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
