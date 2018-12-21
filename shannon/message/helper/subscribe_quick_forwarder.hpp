///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef GALOIS_MESSAGE_QUICKFORWARDER_HPP
#define GALOIS_MESSAGE_QUICKFORWARDER_HPP

#include <iostream>
#include <string>
#include <map>
#include <vector>

#include <shannon/message/server.hpp>
#include "subscribe_observer.h"

namespace Shannon{
	namespace Message{

template<typename DataStruct>
class QuickForwarder : public Observer<SubscribeField>
{
public:
	explicit QuickForwarder( const MessageTypeGroup & messageGroup )
		:Observer<SubscribeField>(messageGroup, false), m_filter_keys(true){}

	explicit QuickForwarder( const MessageTypeGroup & messageGroup, bool filter_keys)
		:Observer<SubscribeField>(messageGroup, false), m_filter_keys(filter_keys){}

	////////ObserveTimeFormat: HH:MM:SS
	virtual void Observe( const char * observeTime, const bool IsTradingTime = true ) {}
	virtual void Init( const char * initTime ){} 
	virtual void SectionForward() {}


	virtual void DirectForward(const DataStruct & );
protected:
	const bool m_filter_keys;
protected:
	virtual void _ReStartNotifyMessenger(const std::shared_ptr<MessengerObserverStruct> & messenger_ptr ){
		std::cout<<" Warning: subscribe mode should be quick mode instead of restart mode"<<std::endl;
		messenger_ptr->resume_type = SHANNON_MESSAGE_RESUME_QUICK;
	}
};



template<typename DataStruct>
void QuickForwarder<DataStruct>::DirectForward(const  DataStruct &data)
{
	boost::lock_guard<boost::shared_mutex> write_lock(messenger_mutex_);

	std::shared_ptr<MessengerBase> messenger_ptr;
	MessageInfo<DataStruct> message_out;
	memcpy(&message_out.content, &data, sizeof(DataStruct));
	for( auto it = this->thread_messenger_subscribe_map_.begin(); it != this->thread_messenger_subscribe_map_.end();)
	{
		std::vector<std::string> & subscribe_keys = it->second->subscribe_keys;
		if( m_filter_keys && ! std::binary_search(  subscribe_keys.begin(), subscribe_keys.end(), data.key_id() )  ){
			++it;
			continue;
		}
		
		
		bool to_be_erased = false;
		if( SHANNON_MESSAGE_RESUME_QUICK == it->second->resume_type ){
			if( (messenger_ptr = it->second->messenger_.lock()) ){
#ifdef SHANNON_MESSAGE_SERVER_DEBUG
				std::cout<<"===> QuickForwarder::DirectForward Send MessageType:"<<message_group_.update<<" for DataStruct:"<<typeid(DataStruct).name()<<std::endl;
#endif
				if( !messenger_ptr->Post( message_out, message_group_.update, true ) ) to_be_erased = true;
			}		
		}

		if( to_be_erased ){
			this->thread_messenger_subscribe_map_.erase( it++ );
		}else{
			++it;
		}
	}
};


}////namespace Message
}////namespace Galois
#endif