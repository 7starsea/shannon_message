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
****************************************************************************
**                     Author: Aimin Huang                                **
**                       Date: 2015.12.04                                 **
**                        Version: 1.0.0                                  **
****************************************************************************/
#ifndef SHANNON_MESSAGE_OBSERVER_SUBSCRIBE_H
#define SHANNON_MESSAGE_OBSERVER_SUBSCRIBE_H


#include <string>
#include <map>
#include <vector>
#include <memory>
#include <boost/thread/shared_mutex.hpp>
#include <thread>

#include <shannon/message/helper/subscribe_struct.h>
#include <shannon/message/observer.hpp>

namespace Shannon{
	namespace Message{


/*Specification for ReceiveStruct=SubscribeField*/
template<>
class Observer<SubscribeField>{
protected:
		class MessengerObserverStruct
		{
		public:
			std::weak_ptr<MessengerBase> messenger_;
			MessageResumeType resume_type;
			bool subscribe_finished;
			std::vector<std::string> subscribe_keys;
			std::vector<std::string> restart_keys;
			MessengerObserverStruct()
				:messenger_(), resume_type(SHANNON_MESSAGE_RESUME_UNKNOWN),subscribe_finished(false)
			{}
		};
public:
	explicit Observer( const MessageTypeGroup & messageGroup, bool support_restart = true );
	virtual ~Observer( );

	int RtnClientSize(){
		boost::shared_lock<boost::shared_mutex> read_lock(messenger_mutex_);
		return (int) thread_messenger_subscribe_map_.size();
	}

	void OnReceivedMsg( int unique_id, const std::weak_ptr<MessengerBase> & pMessenger, SubscribeField * pSubField, const MessageType msgType );
	void EraseMessenger(  int unique_id );
	void CleanMessenger(  int unique_id );

	///// must implement these three methods
	virtual void Init( const char * initTime )  = 0;
	virtual void Observe( const char * observeTime, const bool IsTradingTime = true ) = 0;
	virtual void SectionForward() = 0;

protected:
	virtual void _ReStartNotifyMessenger(const std::shared_ptr<MessengerObserverStruct>& ) = 0;
public:
	const MessageTypeGroup message_group_;
protected:
	const bool support_restart_;

	bool new_restart_messenger_;

	////What I can Observe
	bool observer_keys_sorted_;
	std::vector<std::string> observer_keys_;

	std::map< int, std::shared_ptr<MessengerObserverStruct> > thread_messenger_subscribe_map_;

	//多线程的锁
	boost::shared_mutex messenger_mutex_;
};


}//namespace Message
}//namespace Shannon


#endif