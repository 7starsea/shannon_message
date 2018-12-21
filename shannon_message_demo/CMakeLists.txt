cmake_minimum_required(VERSION 3.2)

project (Shannon.Message.Client.Demo)


if(UNIX)
	set(CMAKE_USE_PTHREADS_INIT "pthread")
endif(UNIX)



set(source_dir ${CMAKE_CURRENT_SOURCE_DIR})

set(client_demo_src
	${source_dir}/demo-client.cpp
	${source_dir}/demo-message-datastruct.h
)

set(server_demo_src
	${source_dir}/demo-server-forwarders.h
	${source_dir}/binary_logger.hpp
	${source_dir}/binary_logger_util.h
	${source_dir}/binary_logger_util.cpp
	${source_dir}/demo-server.cpp
	${source_dir}/demo-message-datastruct.h
)


set(quick_server_demo_src
	${source_dir}/demo-server-forwarders-quick.h
	${source_dir}/demo-server-quick.cpp
	${source_dir}/demo-message-datastruct.h
)



set(current_target "Client.Demo")
add_executable(${current_target} ${client_demo_src} )
TARGET_LINK_LIBRARIES(${current_target} ${shannon_message_lib} ${Boost_LIBRARIES} ${CMAKE_USE_PTHREADS_INIT})


set(current_target "Server.Demo")
add_executable(${current_target} ${server_demo_src})
TARGET_LINK_LIBRARIES(${current_target} ${shannon_message_lib} ${Boost_LIBRARIES} ${CMAKE_USE_PTHREADS_INIT})


set(current_target "QuickServer.Demo")
add_executable(${current_target} ${quick_server_demo_src})
TARGET_LINK_LIBRARIES(${current_target} ${shannon_message_lib} ${Boost_LIBRARIES} ${CMAKE_USE_PTHREADS_INIT})










