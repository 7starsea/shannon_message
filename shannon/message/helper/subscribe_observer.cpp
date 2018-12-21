///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#include "subscribe_observer.h"
#include <iostream>
#include <algorithm>

namespace Shannon{
	namespace Message{



Observer<SubscribeField>::Observer( const MessageTypeGroup & messageGroup,  bool support_restart /*= true */  )
	: message_group_(messageGroup),support_restart_(support_restart) ,new_restart_messenger_(false),observer_keys_sorted_(false)
{ };


Observer<SubscribeField>::~Observer( ){
//	std::cout<<"===> destroy Observer<SubscribeField>"<<std::endl;
};


void Observer<SubscribeField>::OnReceivedMsg( int unique_id, const std::weak_ptr<MessengerBase> & pMessenger, SubscribeField * pSubField, const MessageType msgType  )
{
	if( msgType == message_group_.subscribe || msgType == message_group_.update || msgType == message_group_.unsubscribe ){
		boost::lock_guard<boost::shared_mutex> write_lock(messenger_mutex_);
		auto itMessengerPtr = thread_messenger_subscribe_map_.find( unique_id );
		if( itMessengerPtr == thread_messenger_subscribe_map_.end() ){
			std::shared_ptr<MessengerObserverStruct> t( new MessengerObserverStruct );
			t->messenger_	= pMessenger;
			t->resume_type	= SHANNON_MESSAGE_RESUME_UNKNOWN;
			t->subscribe_finished = false;
			t->subscribe_keys.reserve( observer_keys_.size() );
			t->restart_keys.reserve( observer_keys_.size() );
			auto result = thread_messenger_subscribe_map_.insert( std::make_pair(unique_id, t) );
			if(result.second) itMessengerPtr = result.first;
			else return;
		}
		if( msgType == message_group_.subscribe ){
			if( !observer_keys_sorted_ ){
				std::sort(observer_keys_.begin(), observer_keys_.end());
				observer_keys_sorted_ = true;
			}
#ifndef SHANNON_MESSAGE_DATACENTER_SERVER
			if( std::binary_search( observer_keys_.begin(), observer_keys_.end(), pSubField->key_id() ) )
#endif
			{
				std::vector<std::string> & subscrike_keys = itMessengerPtr->second->subscribe_keys ;
				if(  std::find(  subscrike_keys.begin(), subscrike_keys.end(), pSubField->key_id()   ) == subscrike_keys.end() )
				{
					subscrike_keys.push_back( pSubField->key_id() );
	//				std::cout<<"Add Key:"<<pSubField->key_id<<" to Thread ID:"<<thread_id<<std::endl;
					switch ( pSubField->resume_type ){
					case SHANNON_MESSAGE_RESUME_RESTART:
						if( support_restart_ ){
							itMessengerPtr->second->restart_keys.push_back( pSubField->key_id() );
							new_restart_messenger_ = true;
							if( SHANNON_MESSAGE_RESUME_RESTART != itMessengerPtr->second->resume_type )
								itMessengerPtr->second->resume_type = SHANNON_MESSAGE_RESUME_RESTART;
						}
						break;
					case SHANNON_MESSAGE_RESUME_QUICK:
						if( SHANNON_MESSAGE_RESUME_RESTART != itMessengerPtr->second->resume_type  )
							itMessengerPtr->second->resume_type = SHANNON_MESSAGE_RESUME_QUICK;
						break;
					default:
						break;
					}

					std::sort(itMessengerPtr->second->subscribe_keys.begin(), itMessengerPtr->second->subscribe_keys.end());
					std::sort(itMessengerPtr->second->restart_keys.begin(), itMessengerPtr->second->restart_keys.end());
				}
			}
#ifndef SHANNON_MESSAGE_DATACENTER_SERVER
#ifdef SHANNON_MESSAGE_SERVER_DEBUG		
			else{		
				std::shared_ptr<MessengerBase> ptr;
				if( ( ptr= itMessengerPtr->second->messenger_.lock() ) )
					std::cout<<"===> Subscribed unknow key:"<<pSubField->key_id()<<" from Client:"<<ptr->RtRemoteAddress()<<std::endl;
			}
#endif
#endif
		}else if( msgType == message_group_.unsubscribe ){
			std::vector<std::string> & subscrike_keys = itMessengerPtr->second->subscribe_keys ;
			auto it = std::find( subscrike_keys.begin(), subscrike_keys.end(), pSubField->key_id() ) ;
			if( it	!=  subscrike_keys.end() ){
				subscrike_keys.erase( it );
			}
		}

	}
};


void Observer<SubscribeField>::EraseMessenger(  int unique_id  ){
	boost::lock_guard<boost::shared_mutex> write_lock(messenger_mutex_);
	thread_messenger_subscribe_map_.erase( unique_id );
};


///// if some thread_id does not contain any subscribe key, then erase this thread_id
void Observer<SubscribeField>::CleanMessenger( int unique_id  )
{	
	boost::lock_guard<boost::shared_mutex> write_lock(messenger_mutex_);
	for( auto it = thread_messenger_subscribe_map_.begin(); it != thread_messenger_subscribe_map_.end(); )
	{
		if( it->first == unique_id ){
			if( !it->second->subscribe_keys.size() ){
#ifdef SHANNON_MESSAGE_SERVER_DEBUG
				std::shared_ptr<MessengerBase> ptr= it->second->messenger_.lock();
				if(  ptr  )
					std::cout<<"===> In CleanMessenger Erased Client:"<<ptr->RtRemoteAddress()<<std::endl;
#endif
				thread_messenger_subscribe_map_.erase( it++ );
				continue;
			}
			{
				std::shared_ptr<MessengerBase> ptr= it->second->messenger_.lock();
				if(  ptr  ){
					std::cout<<"===> Number of Received Keys: "<<it->second->subscribe_keys.size()<<" for Client: "<<ptr->RtRemoteAddress()<<std::endl;
				}
			}
			std::sort(it->second->subscribe_keys.begin(), it->second->subscribe_keys.end());
			std::sort(it->second->restart_keys.begin(), it->second->restart_keys.end());
			it->second->subscribe_finished = true;
			if( support_restart_ && SHANNON_MESSAGE_RESUME_RESTART == it->second->resume_type && it->second->restart_keys.size() ){
				_ReStartNotifyMessenger( it->second );
			}
		}
		++it;
	}
}


}//namespace Message
}//namespace Shannon
