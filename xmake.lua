add_rules("mode.debug", "mode.release")
set_languages("cxx20")

option("static", {showmenu=true, default=true})
option("shared", {showmenu=true, default=false})
option("sse", {showmenu=true, default=false })
option("avx", {showmenu=true, default=false})

target("libhat")
    set_languages("cxx20")
    if has_config("static") then 
        set_kind("static")
    else
        set_kind("shared")
        add_defines("LIBHAT_BUILD_SHARED_LIB")
    end
    if has_config("sse") then
        print("Using SSE")
    else
        add_defines("LIBHAT_DISABLE_SSE")
    end

    if has_config("avx") then 
        print("using AVX")
    else
        add_defines("LIBHAT_DISABLE_AVX512")
    end
    add_includedirs("libhat/include")
    add_files("libhat/src/**.cpp")

target("oxorany")
    set_languages("cxx20")
    set_kind("static")
    add_files("oxorany/*.cpp")
    add_includedirs("oxorany")


target("UESDK")
    set_languages("cxx20")
    add_deps("libhat")
    set_kind("static")
    add_includedirs("libhat/include","uesdk/include", "uesdk/src")
    add_files("uesdk/src/**.cpp")



target("GFSDK_Aftermath_Lib.x64")
    set_symbols("debug")
    add_deps("UESDK", "libhat", "oxorany")
    set_kind("shared")
    add_links("detours/lib/detours.lib", "user32")
    add_includedirs("uesdk/include", "detours/include", "dll", "oxorany", "uesdk/src")
    add_files("dll/*.cpp")
