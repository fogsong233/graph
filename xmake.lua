add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})
set_toolchains("clang")
set_languages("c++23")
add_requires("gtest", {configs = {cxflags = "-stdlib=libc++"}, system=false })
 add_ldflags("-lc++", "-lc++abi", {force = true})
    add_links("c++", "c++abi")


target("graph")
    set_kind("binary")
    add_includedirs("src/graph")
    add_packages("gtest")
    add_files("src/**/*.cpp")
    add_files("src/*.cpp")

