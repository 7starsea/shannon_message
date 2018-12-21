///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
/***************************************************************************
**                                                                        **
**					A simple template message server class                **
**       Can be easily used in intra-system message communications        **
**      Must work in coordination with a corresponding message client     **
**                                                                        **
****************************************************************************/
#ifndef SHANNON_MESSAGE_SERVER_BASE_H
#define SHANNON_MESSAGE_SERVER_BASE_H

#include <boost/atomic.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <thread>

#include "data_struct.h"
#include "unified_send_fun.h"
#include "instance_container.hpp"


namespace Shannon{
	namespace Message{

class MessengerBase;
typedef Container< std::shared_ptr<MessengerBase> > MessengerBaseContainer;

class MessengerBase : public std::enable_shared_from_this<MessengerBase>
{
private:
	static  int sm_total_clients_;
	static std::map<std::string,  int> sm_client_failtimes_;
public:
	const int unique_id_;
protected:
	std::shared_ptr<boost::asio::ip::tcp::socket>  socket_;
	MessengerBaseContainer & container_;
	const std::function< void(int, const std::string &) >  logger_;
	
	std::string remote_address_;
	std::string remote_address_port_;
	std::mutex socket_mutex_;
protected:
	void _Init();
	void _Remove();

public:
	MessengerBase( boost::asio::io_service & io_service,  MessengerBaseContainer & container,const std::function< void(int, const std::string &) > & logger );
	virtual ~MessengerBase();

	inline std::string RtRemoteAddress()const{ return this->remote_address_port_; }

	template<typename SendStruct>
	bool Post( const MessageInfo<SendStruct> & message, const MessageType type_info, bool sentImmediately=false);

	virtual void Init() = 0;	

	inline std::shared_ptr<boost::asio::ip::tcp::socket> socket(){	return socket_; }
};

template<typename SendStruct>
bool MessengerBase::Post( const MessageInfo<SendStruct> & msg, const MessageType type_info, bool /*sentImmediately=false, reserved for future update*/ )
{
	bool isPost = false;
	if( socket_->is_open() ){
		std::lock_guard<std::mutex> guard(socket_mutex_);
		
		char buffer_send_data[sizeof(MessageType)+sizeof(MessageInfo<SendStruct>)];
		std::size_t transferred_size = sizeof( MessageType );
		memcpy( buffer_send_data, (&type_info), sizeof( MessageType ) );
		if(  type_info > 0 ){
			transferred_size += sizeof(MessageInfo<SendStruct>);
			memcpy( buffer_send_data + sizeof( MessageType ), (&msg), sizeof(MessageInfo<SendStruct>) );
		}
		isPost = UnifiedSendMessage(socket_, buffer_send_data, transferred_size );

		if( !isPost ){
			logger_(container_.size(), "Send Message failed in messenger:" + boost::lexical_cast<std::string>( unique_id_ ) );
			sm_client_failtimes_.at( remote_address_ ) +=1;
		}
	}else{		
		/////notify
		logger_(container_.size(), "I am still running... but the socket is closed in messenger:" + boost::lexical_cast<std::string>( unique_id_ ) );
	}
	if( !isPost ) _Remove();
	return isPost;
};






/*Message Server*/
class ServerBase  
{
public:
    ServerBase( boost::asio::ip::tcp::endpoint &endpoint,const std::function< void(int, const std::string &) > & logger  );
	ServerBase( unsigned short port, const std::function< void(int, const std::string &) > & logger  );
	virtual ~ServerBase(){}
    void run( bool detached = false) ;
	void stop();
	void heart_beating();
	void heart_beating_loop(int sec);
	inline bool exists_client(){return container_.size() > 0 || running_threads_.load(boost::memory_order_consume);}
protected:
	virtual void _Listen() = 0;
	void _enter_run_loop(bool);	
	void _heart_beating_loop(int sec);
private:
	void __init( boost::asio::ip::tcp::endpoint &endpoint );
protected:
	boost::asio::io_service  io_service_; 
	boost::asio::ip::tcp::acceptor acceptor_;
	const std::function< void(int, const std::string &) > logger_;
	MessengerBaseContainer container_;

	boost::atomic<std::size_t> running_threads_;
	boost::atomic<bool> stopped_;
};





}//namespace Message
}//namespace Shannon


#endif