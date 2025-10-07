add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})
set_toolchains("clang")
set_languages("c++23")
add_ldflags("-stdlib=libc++", "-lc++", "-lc++abi", {force = true})
add_cxflags("-stdlib=libc++", {force = true})
add_requires("gtest", {configs = {
    toolchains = "clang", 
    cxflags = "-stdlib=libc++",
    ldflags = "-stdlib=libc++"  
}})


target("graph")
    set_kind("binary")
    add_includedirs("src/graph")
    add_packages("gtest")
    add_files("src/**/*.cpp")
    add_files("src/*.cpp")

