///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef SHANNON_MESSAGE_DATA_TYPE_H
#define SHANNON_MESSAGE_DATA_TYPE_H



#define SHANNON_MESSAGE_CONNECTIONS_ALLOWED 0 ///0 represents infinity connnectons allowed

#define SHANNON_MESSAGE_CHECK_FIELD			12356///any short number
/////////////////////////////////////////////////////////////////////////
//消息传输模式字段
/////////////////////////////////////////////////////////////////////////
#define SHANNON_MESSAGE_RESUME_RESTART '0'
#define SHANNON_MESSAGE_RESUME_QUICK   '1'
#define SHANNON_MESSAGE_RESUME_UNKNOWN '2'



//消息类型字段
#define SHANNON_MESSAGE_NO_MORE_TYPE		 0
#define SHANNON_MESSAGE_NULL_VOID_TYPE		 -1
#define SHANNON_MESSAGE_ACTIVE_CLOSE_TYPE	 -2
#define SHANNON_MESSAGE_HEART_BEATING_TYPE	 -3

namespace Shannon{
	namespace Message{
		//消息是否重传
		typedef char MessageResumeType;

		/////////////////////////////////////////////////////////////////////////
		//消息类型字段
		/////////////////////////////////////////////////////////////////////////
		typedef int MessageType;
}//namespace Message
}//namespace Shannon

#endif