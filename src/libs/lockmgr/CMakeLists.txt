# 将本目录下的所有文件全部编译成静态库
aux_source_directory(. LOCK_MGR)

add_library(lockmgr STATIC ${LOCK_MGR})