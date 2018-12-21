///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef SHANNON_MESSAGE_DATA_STRUCT_H
#define SHANNON_MESSAGE_DATA_STRUCT_H
#include "data_type.h"
#include <string.h>

namespace Shannon{
	namespace Message{


   typedef struct MessageTypeGroup
    {
        MessageType subscribe;
        MessageType unsubscribe;
        MessageType update;
		MessageTypeGroup()
			:subscribe(SHANNON_MESSAGE_NULL_VOID_TYPE),
			unsubscribe(SHANNON_MESSAGE_NULL_VOID_TYPE),
			update(SHANNON_MESSAGE_NULL_VOID_TYPE)
		{}

		bool operator==( const MessageTypeGroup &other ) const{
			return subscribe == other.subscribe && unsubscribe == other.unsubscribe && update == other.update;
		}
    }MessageTypeGroup;


    //与 Generator通信的消息体
     template <typename MsgStruct>
      struct MessageInfo {
        MsgStruct	content;
		const short check_field;
		MessageInfo()
			:content(),check_field(SHANNON_MESSAGE_CHECK_FIELD)
		{	}
     };


}//namespace Message
}//namespace Shannon



#endif