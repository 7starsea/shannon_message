///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef GALOIS_MESSAGE_FORWARDER_HPP
#define GALOIS_MESSAGE_FORWARDER_HPP

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <boost/mpl/bool.hpp>

#include <boost/timer/timer.hpp>

#include <shannon/message/server.hpp>
#include "subscribe_observer.h"

/*
BLogger must provide the following functions:
	concept behind: 2 level cache;

readFromFirstCache( std::vector<DataStruct>&, bool )
readFromSecondCache( std::vector<DataStruct>&, int, int )
enableAutoWriteToSecondCache( bool )
append( DataStruct )
append( std::vector<DataStruct> )
recover( int )

readLast(  std::vector<DataStruct> )
appendLast(  std::vector<DataStruct> )
*/
namespace Shannon{
	namespace Message{
template<typename DataStruct, class BLogger, bool DirectForwardParam>
class Forwarder : public Observer<SubscribeField>
{
public:
	explicit Forwarder( const MessageTypeGroup & messageLoop, BLogger * logger );
	explicit Forwarder( const MessageTypeGroup & messageLoop, BLogger * logger, bool filter_keys );
	virtual ~Forwarder(){ 	}//std::cout<<"===> destroy galois forwarder"<<std::endl;
	////////ObserveTimeFormat: HH:MM:SS
	virtual void Observe( const char * observeTime, const bool IsTradingTime = true ){ (void)(observeTime); (void)(IsTradingTime);}
	virtual void Init( const char * initTime ){
		(void)(initTime);
		//////检测已有数据的完整性，并判断文件是否为空
		 m_logger->recover( 1 ) ;
	}

	virtual void SectionForward();
	virtual void DirectForward(const DataStruct & );
protected:
	virtual void _ReStartNotifyMessenger(const std::shared_ptr<MessengerObserverStruct>& ) ;

protected:
	BLogger * m_logger;
	const bool m_filter_keys;
private:
	bool ___SendVecData(const std::vector<DataStruct> & vData, std::shared_ptr<MessengerBase> & messenger_ptr, const std::vector<std::string> & subscribe_keys);
};


template<typename DataStruct, class BLogger, bool DirectForwardParam>
Forwarder<DataStruct, BLogger, DirectForwardParam>::Forwarder(const MessageTypeGroup & messageLoop, BLogger * logger )
	:Observer<SubscribeField>(messageLoop), m_logger(logger), m_filter_keys(true)
{ };

template<typename DataStruct, class BLogger, bool DirectForwardParam>
Forwarder<DataStruct, BLogger, DirectForwardParam>::Forwarder(const MessageTypeGroup & messageLoop, BLogger * logger, bool filter_keys )
	:Observer<SubscribeField>(messageLoop), m_logger(logger), m_filter_keys(filter_keys)
{ };

template<typename DataStruct, class BLogger, bool DirectForwardParam>
bool Forwarder<DataStruct, BLogger, DirectForwardParam>::___SendVecData(const std::vector<DataStruct> & vData,
																			  std::shared_ptr<MessengerBase> & messenger_ptr,
																		const std::vector<std::string> & subscribe_keys)
{
	bool iResult = true;
	MessageInfo<DataStruct> message_out;
	for( auto it1 = vData.begin(); it1 != vData.end(); ++it1 ){

		if( m_filter_keys && ! std::binary_search( subscribe_keys.begin(), subscribe_keys.end(), it1->key_id() )  )
			continue;
		memcpy(&message_out.content, &(*it1), sizeof(DataStruct));
		if( !messenger_ptr->Post( message_out, message_group_.update ) ){
			iResult=false;break;
		}
		
	}
	return iResult;
}
template<typename DataStruct, class BLogger, bool DirectForwardParam>
void Forwarder<DataStruct, BLogger, DirectForwardParam>::_ReStartNotifyMessenger(const std::shared_ptr<MessengerObserverStruct> & messenger_observer)
{
		if( this->new_restart_messenger_ )
		{//////None Precise Position BinaryDataLogger
			if(  boost::mpl::bool_<DirectForwardParam>::value )		m_logger->enableAutoWriteToSecondCache( false );

			bool has_unfinished_messenger = false;

			for( auto it = this->thread_messenger_subscribe_map_.begin(); it != this->thread_messenger_subscribe_map_.end();++it)
			{
				has_unfinished_messenger = has_unfinished_messenger || (SHANNON_MESSAGE_RESUME_RESTART == it->second->resume_type && !it->second->subscribe_finished);
			}
		
			
			std::shared_ptr<MessengerBase> messenger_ptr;
			if( (messenger_ptr = messenger_observer->messenger_.lock()) ){
			
				unsigned int max_count = 2097152/sizeof(DataStruct), readed=0; //=2*1024*1024/sizeof(DataStruct) = 2 MegaBytes / sizeof(DataStruct) 

				std::vector<DataStruct> vData;
				vData.reserve( max_count );
				bool iResult = true;
				do{
					vData.resize(0);
					m_logger->readFromSecondCache( vData, readed, 1, max_count ); ////
					iResult = ___SendVecData(vData, messenger_ptr, messenger_observer->restart_keys);
					readed += vData.size();
				}while( max_count == vData.size() && iResult );
				
				if(  boost::mpl::bool_<DirectForwardParam>::value && iResult){
					vData.resize(0);
					m_logger->readFromFirstCache( vData );	
					___SendVecData(vData, messenger_ptr, messenger_observer->restart_keys);
				}
				messenger_observer->resume_type =  SHANNON_MESSAGE_RESUME_QUICK;
			}else{
				messenger_observer->resume_type =  SHANNON_MESSAGE_RESUME_UNKNOWN;
			}

			if( !has_unfinished_messenger ){
				this->new_restart_messenger_ = false;
				if(  boost::mpl::bool_<DirectForwardParam>::value ) m_logger->enableAutoWriteToSecondCache( true );
			}
		}
}


template<typename DataStruct, class BLogger, bool DirectForwardParam>
void Forwarder<DataStruct, BLogger, DirectForwardParam>::DirectForward(const  DataStruct &data)
{

	if( ! boost::mpl::bool_<DirectForwardParam>::value ) return;
	boost::lock_guard<boost::shared_mutex> write_lock(messenger_mutex_);
	if( this->new_restart_messenger_ )	{
		m_logger->enableAutoWriteToSecondCache( false );
	}

	m_logger->append( data );
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
		if( (messenger_ptr = it->second->messenger_.lock()) ){
			switch (it->second->resume_type)
			{
			case SHANNON_MESSAGE_RESUME_RESTART:			
				break;
			case SHANNON_MESSAGE_RESUME_QUICK:	
						if( !messenger_ptr->Post(message_out, message_group_.update, true ) ) to_be_erased = true;
				break;
			case SHANNON_MESSAGE_RESUME_UNKNOWN:
				break;
			default:
				std::cout<<"Unexpected runtime error happened in DirectForward"<<std::endl;
				break;
			}
		}else{
			to_be_erased = true;
		}
		if( to_be_erased ){
			std::cout<<"===> Forwarder::DirectForward erased messenger: "<<it->first<<" Logger:"<<m_logger->m_logger_name<<std::endl;
			this->thread_messenger_subscribe_map_.erase( it++ );
		}else{
			++it;
		}
	}
};



template<typename DataStruct, class BLogger, bool DirectForwardParam>
void Forwarder<DataStruct, BLogger, DirectForwardParam>::SectionForward( )
{
	if( boost::mpl::bool_<DirectForwardParam>::value ) return;

	boost::lock_guard<boost::shared_mutex> write_lock(messenger_mutex_);

	std::vector<DataStruct> pendingData;
	m_logger->readFromFirstCache( pendingData );
	for( auto it = this->thread_messenger_subscribe_map_.begin(); it != this->thread_messenger_subscribe_map_.end(); )
	{
		bool to_be_erased = false;
		
		std::shared_ptr<MessengerBase> messenger_ptr;
		if( (messenger_ptr = it->second->messenger_.lock()) ){
			if( SHANNON_MESSAGE_RESUME_QUICK == it->second->resume_type ){
				//考虑实际应用（一般会订阅所有Key）和程序速度，也会发送未订阅的Key数据
				to_be_erased = !___SendVecData(pendingData, messenger_ptr, it->second->subscribe_keys);
				
			}
		}else{
			to_be_erased=true;
		}
		if( to_be_erased ){
			std::cout<<"===> Forwarder::SectionForward erased messenger:"<<it->first<<std::endl;
			this->thread_messenger_subscribe_map_.erase( it++ );
		}else{
			++it;
		}
	}
};

}////namespace Message
}////namespace Galois
#endif