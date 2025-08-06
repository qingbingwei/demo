include_directories(Classes/views)
add_subdirectory(views) 
# 假设主项目的源文件变量名为 PROJECT_SRC
set(PROJECT_SRC
    ${TEST-cursor_SRC}
    ${VIEWS_SRC}
    PARENT_SCOPE
)