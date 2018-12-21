///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef SHANNON_MESSAGE_SUBSCRIBE_DATA_STRUCT_H
#define SHANNON_MESSAGE_SUBSCRIBE_DATA_STRUCT_H

#include <shannon/message/data_type.h>

namespace Shannon{
	namespace Message{


    typedef struct SubscribeField{
        //需要订阅的合约
        char 	id[31];
        MessageResumeType resume_type;
		const char * key_id() const{
			return this->id;
		}	
    }SubscribeField;

}//namespace Message
}//namespace Shannon
#endif