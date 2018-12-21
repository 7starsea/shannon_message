

macro(shannon_init_compiler_system)
	if(WIN32)
		if(CMAKE_CONFIGURATION_TYPES)
			set(CMAKE_CONFIGURATION_TYPES Debug Release)
		 endif()

		message(status "Setting MSVC flags")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /WX- /D UNICODE /wd4819")
		add_definitions(-D_SCL_SECURE_NO_WARNINGS)
		add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	endif(WIN32)

	if(UNIX)	
		add_definitions(-Wall)
		include(CheckCXXCompilerFlag)
		CHECK_CXX_COMPILER_FLAG(-std=c++11 HAVING_COMPILER_SUPPORTS_CXX11)
		if(HAVING_COMPILER_SUPPORTS_CXX11)
			add_definitions(-std=c++11)
		else(HAVING_COMPILER_SUPPORTS_CXX11)
			CHECK_CXX_COMPILER_FLAG(-std=c++0x HAVING_COMPILER_SUPPORTS_CXX0X)
			if(HAVING_COMPILER_SUPPORTS_CXX0X)
				add_definitions(-std=c++0x)
			else(HAVING_COMPILER_SUPPORTS_CXX0X)
				message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
			endif(HAVING_COMPILER_SUPPORTS_CXX0X)
		endif(HAVING_COMPILER_SUPPORTS_CXX11)
	endif(UNIX)
endmacro(shannon_init_compiler_system)

macro(shannon_init_boost_env)
    if(UNIX)
        set(CMAKE_USE_PTHREADS_INIT "pthread")
        add_definitions(-fPIC)
    endif(UNIX)

    if(WIN32)
        set(Boost_USE_MULTITHREADED      ON)
        set(Boost_USE_STATIC_LIBS        ON)
        set(Boost_USE_STATIC_RUNTIME     OFF)
    else()
        set(Boost_USE_MULTITHREADED  ON)
        SET(Boost_USE_STATIC_LIBS OFF)
        SET(Boost_USE_STATIC_RUNTIME OFF)
    endif()
endmacro(shannon_init_boost_env)

function(FindMyBuildLib api_lib_var lib_path lib_name)
    if(WIN32)
        set(my_api_lib
        debug ${lib_path}/${lib_name}d.lib 
        optimized ${lib_path}/${lib_name}.lib 
        )
    else(WIN32)
        set(my_api_lib 
        debug ${lib_path}/lib${lib_name}d.a 
        optimized ${lib_path}/lib${lib_name}.a
        )
    endif(WIN32)

    set(${api_lib_var} ${my_api_lib} PARENT_SCOPE)
endfunction(FindMyBuildLib)
