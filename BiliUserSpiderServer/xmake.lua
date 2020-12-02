target("bilispider_server")
    set_kind("binary")
    -- std=c++11
    set_languages("c99", "cxx11")
    -- include dir
    add_includedirs("src/log")
    -- source file
    add_files("src/**.cpp")
    add_files("src/**.c")
    -- link flags
    add_linkdirs("/usr/local/lib/spdlog")
    add_links("spdlog", "mysqlclient", "curl", "pthread", "jsoncpp")
    -- build dir
    set_targetdir("$(projectdir)/bin")
    set_objectdir("build/objs")
    --add_mflags("-g", "-O2", "-DDEBUG")
    add_mflags("-O2")

