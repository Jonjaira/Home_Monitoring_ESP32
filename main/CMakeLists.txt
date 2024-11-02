# Add sources from ui directory
file(GLOB_RECURSE SRC_UI "ui/*.c")

idf_component_register(
    SRCS "main.c" "sockets.c" ${SRC_UI} "cJSON.c"
    INCLUDE_DIRS "." "ui"
)
