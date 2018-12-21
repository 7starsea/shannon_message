///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef SHANNON_MESSAGE_UNIFIED_FUNCTIONS_H
#define SHANNON_MESSAGE_UNIFIED_FUNCTIONS_H

#include <iostream>
#include <string>
#include <memory>
#include <boost/asio.hpp>
#include <boost/predef.h>

namespace Shannon{
	namespace Message{


#if BOOST_OS_UNIX || BOOST_OS_LINUX || BOOST_OS_MACOS
#include <time.h>   // Needed for struct timespec on linux and unix and macos
		//nanosleep((const struct timespec[]){{0, milli * 1000000L}},  NULL);
		inline void UnifiedSleep(int milli){const struct timespec tim = { milli/1000, (milli%1000) * 1000000L};   nanosleep(&tim,  NULL); }
#elif BOOST_OS_WINDOWS	////on windows
#include <Windows.h>
		inline void UnifiedSleep(int milli){ Sleep(milli); }
#elif __cplusplus >= 199711L
#include <thread>         // include std::this_thread::sleep_for and std::chrono::seconds
		inline void UnifiedSleep(int milli){ std::this_thread::sleep_for(std::chrono::milliseconds( milli ));}
#endif





void UnifiedLoggerForClient(int data_received, const std::string & msg);
void UnifiedLoggerForServer(int clients, const std::string & msg);

////Note: Please set non_blocking option for the tcp socket
bool UnifiedSendMessage(const std::shared_ptr<boost::asio::ip::tcp::socket> & tcp_socket, 
						const void * msg, 
						const std::size_t bytes_need_transferred, 					
						const int retry_interval_in_millisec = 50 /* 50 milliseconds*/);

}//namespace Message
}//namespace Shannon

#endif