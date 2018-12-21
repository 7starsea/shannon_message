///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
/***************************************************************************
**                                                                        **
**					A simple template message observer class              **
**       Can be easily used in intra-system message communications        **
**      Must work in coordination with a corresponding message server     **
**                                                                        **
****************************************************************************/
#ifndef SHANNON_MESSAGE_OBSERVER_H
#define SHANNON_MESSAGE_OBSERVER_H

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <boost/thread/shared_mutex.hpp>
#include <thread>

#include "server_base.h"
namespace Shannon{
	namespace Message{


template<typename ReceiveStruct>
class Observer{
public:
	explicit Observer( const MessageTypeGroup & messageGroup, const std::function< void(ReceiveStruct*, const MessageType, int )> & updater )
		:message_group_(messageGroup), updater_(updater){};
	virtual ~Observer( ){};
	
	void OnReceivedMsg(  int unique_id, const std::weak_ptr<MessengerBase> & pMessenger, ReceiveStruct * pSubField, const MessageType msgType );
	
	void EraseMessenger(   int unique_id ){
		boost::lock_guard<boost::shared_mutex> write_lock(messenger_mutex_);
		thread_messenger_subscribe_map_.erase( unique_id );
	}
	void CleanMessenger(   int unique_id ) { G_UNUSED(unique_id);}

	template<typename SendStruct>
	bool SendMessage(const SendStruct & data, int unique_id);
public:
	const MessageTypeGroup message_group_;
protected:
	const std::function< void(ReceiveStruct*, const MessageType, int )>  updater_;

	///
	std::map< int, std::weak_ptr<MessengerBase> > thread_messenger_subscribe_map_;

	///多线程的锁
	boost::shared_mutex messenger_mutex_;
};

template<typename ReceiveStruct>
void Observer<ReceiveStruct>::OnReceivedMsg( int unique_id, const std::weak_ptr<MessengerBase> & pMessenger, ReceiveStruct * pSubField, const MessageType msgType  )
{
	if( msgType == message_group_.subscribe || msgType == message_group_.update || msgType == message_group_.unsubscribe ){
		boost::lock_guard<boost::shared_mutex> write_lock(messenger_mutex_);
		auto itMessengerPtr = thread_messenger_subscribe_map_.find( unique_id );
		if( itMessengerPtr == thread_messenger_subscribe_map_.end() ){
			thread_messenger_subscribe_map_.insert( std::make_pair(unique_id, pMessenger ) );
		}
		this->updater_(pSubField, msgType, unique_id);
	}
};

template<typename ReceiveStruct>	
template<typename SendStruct>
bool Observer<ReceiveStruct>::SendMessage(const SendStruct & data, int unique_id){
	MessageInfo<SendStruct> msg;
	memcpy(&msg.content, &data, sizeof(SendStruct));
	bool isSent = true;

	boost::lock_guard<boost::shared_mutex> write_lock(messenger_mutex_);
	if( unique_id > 0 ){
		auto it = thread_messenger_subscribe_map_.find( unique_id );
		if( it != thread_messenger_subscribe_map_.end() ){
			std::shared_ptr<MessengerBase> messenger_prt = it->second.lock();	
			if( !(messenger_prt &&  messenger_prt->Post(msg, message_group_.update)) ){
				isSent = false;
				std::cout<<"===> In Observer::SendMessage erased messenger:"<<it->first<<std::endl;
				this->thread_messenger_subscribe_map_.erase( it++ );
			}
		}
	}else{
		for( auto it = this->thread_messenger_subscribe_map_.begin(); it != this->thread_messenger_subscribe_map_.end();){
			std::shared_ptr<MessengerBase> messenger_prt = it->second.lock();	
			if( !(messenger_prt &&  messenger_prt->Post(msg, message_group_.update)) ){
				isSent = false;
				std::cout<<"===> In Observer::SendMessage erased messenger:"<<it->first<<std::endl;
				this->thread_messenger_subscribe_map_.erase( it++ );
			}else{
				++it;
			}
		}
	}
	return isSent;
}


}//namespace Message
}//namespace Shannon


#endif