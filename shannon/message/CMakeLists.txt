cmake_minimum_required(VERSION 3.2)

project (Shannon.Message.Library)

set(source_dir ${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES( ${source_dir}/../../ )

# depends on libraries: boost-system boost-thread
add_definitions(-DBOOST_REGEX_NO_LIB)
set(header 
	${source_dir}/async_client.h
	${source_dir}/server_base.h
	${source_dir}/client_sequence_impl.hpp
	${source_dir}/client_base.h
	${source_dir}/server_sequence_impl.hpp
	${source_dir}/server.hpp
	${source_dir}/client.hpp
	${source_dir}/observer.hpp
    ${source_dir}/unified_send_fun.h
)
set(source 
	${source_dir}/client_base.cpp
	${source_dir}/server_base.cpp
	${source_dir}/unified_send_fun.cpp
	${source_dir}/async_client.cpp)
	
set(helper_header 
	${source_dir}/helper/subscribe_client.hpp
	${source_dir}/helper/subscribe_forwarder.hpp
	${source_dir}/helper/console_client_initializer.hpp
	${source_dir}/helper/subscribe_quick_forwarder.hpp
)	

set(helper_source ${source_dir}/helper/subscribe_observer.cpp)	


set(current_target "shannon_message")
add_library(${current_target} ${source} ${header} ${helper_source} ${helper_header})
set_target_properties(${current_target} PROPERTIES DEBUG_POSTFIX d)
TARGET_LINK_LIBRARIES(${current_target}  ${Boost_LIBRARIES})  


# set(current_target "shannon_message_datacenter")
# add_library(${current_target} STATIC ${source} ${header} ${helper_source}  ${helper_header})
# set_target_properties(${current_target} PROPERTIES DEBUG_POSTFIX d)
# target_compile_definitions(${current_target} PRIVATE SHANNON_MESSAGE_DATACENTER_SERVER) 
# TARGET_LINK_LIBRARIES(${current_target} PUBLIC  ${Boost_LIBRARIES})  


