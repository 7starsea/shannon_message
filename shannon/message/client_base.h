///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
//////////////////////////////////////////////////////////////////////////  
///
///
/// @library			Shannon.Message
/// @author				Aimin Huang
/// @update date		2017-03-05
///  
///  
////////////////////////////////////////////////////////////////////////// 
#ifndef SHANNON_MESSAGE_CLIENT_BASE_H
#define SHANNON_MESSAGE_CLIENT_BASE_H

#include <string>
#include <map>
#include <boost/atomic.hpp>

#include <memory>

#include <thread>
#include <functional>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <algorithm>

#include "data_struct.h"
#include "unified_send_fun.h"
#include "instance_container.hpp"

namespace Shannon{
	namespace Message{

/*!
\goal: 实现传输c-style的struct数据段，传输数据协议为
int + struct
其中 int 类型的数据指示了具体的struct类型，接着传输整个struct数据段

\brief 消息客户端基础类
基础功能：建立与服务端连接及断开
	
所有继承 ClientBase 的具体实现类，建议重载虚函数与监听自己想要收到的数据。
virtual void _Listen();
*/
class ClientBase : public std::enable_shared_from_this<ClientBase>
{
protected:
	class ClientHelper : public Container< std::shared_ptr<ClientBase> >{
	public:
		ClientHelper(std::size_t max_threads);
		virtual ~ClientHelper();
		void run_once();
		void stop();
        void push(const std::shared_ptr<ClientBase> & session );
		inline boost::asio::io_service & get_io_service(){return work_io_service_;}
	private:
		std::size_t max_threads_;
		boost::atomic<std::size_t> running_threads_;
		boost::atomic<bool> stopped_;

		boost::asio::io_service work_io_service_;
		std::shared_ptr<boost::asio::io_service::work> worker_;
		std::shared_ptr<std::thread> work_thread_;

		void _run_loop(bool flag);
};
public:
	static ClientHelper sm_client_helper;
	static void safe_exit();
public:
	///构造函数，需要给出Server的地址，消息类型组，是否不间断打印日志和Logger函数
	ClientBase(const std::string & listen_addr,  /*地址格式，例如: 127.0.0.1:8000*/
					const std::function< void(int, const std::string &) > & logger 
					);
	//析构函数
	virtual ~ClientBase();

	///开始监听消息
	void Listen();
	inline void Connect(){Listen();}

	/// 断开连接
	void stop();
	/// 是否与服务器建立了连接
	inline bool is_connected(){ return is_connected_.load(boost::memory_order_consume);}
public:
	const std::string listen_addr_;								///消息源地址
protected:
	std::shared_ptr<boost::asio::ip::tcp::socket> socket_;		//tcp连接的socket
private:
	boost::asio::deadline_timer timeout_deadline_;
	boost::asio::deadline_timer try_connect_deadline_;
	
	///消息源地址的tcp endpoint格式
	boost::asio::ip::tcp::endpoint endpoint_;
protected:
	///收到消息数量
	int data_received_;
	short connect_timeout_;
	
	MessageType	buffer_type_;

	///标识是否建立了连接
	boost::atomic<bool> is_connected_;
	///是否自动重连
	boost::atomic<bool> auto_reconnected_;
	///logger
	const std::function< void(int, const std::string &) > logger_;

protected:
	//监听，定义为虚函数，需要子类继承
	virtual void _Listen();

	//实现异步的对监听到的消息的处理
	void _OnListenType(const boost::system::error_code& error, std::size_t bytes_transferred );

	//重连函数
	void _ReConnect( );
private:
	//负责与连接相关的细节处理
	void __Connect( );
	void ___Connect( const boost::system::error_code& error);
	void __OnConnect(const boost::system::error_code& error );
	void __CheckDeadline(const boost::system::error_code& error );
};



class ClientBaseSync : public ClientBase{
public:
	///构造函数，需要给出Server的地址，日志Logger函数
	ClientBaseSync(const std::string & listen_addr,  /*地址格式，例如: 127.0.0.1:8000*/
					const std::function< void(int, const std::string &) > logger = &UnifiedLoggerForClient 
					):ClientBase(listen_addr, logger){}

	virtual ~ClientBaseSync(){}


	bool SendMessage( MessageType msg_type ){
		////message type should not be greater than zero
		bool isSent = UnifiedSendMessage(socket_, (void*)&msg_type, sizeof( MessageType ) );
		if( ! isSent ){
			logger_( -data_received_-1, std::string("ClientBase::SendMessage failed for message type.") );
			is_connected_.store(false);
		}
		return isSent;	
	}

	////发送消息
	template<class DataStruct>
	bool SendMessage( const DataStruct * message, MessageType msg_type ){
		MessageInfo<DataStruct> msg;
		if( message ){
			memcpy(&msg.content, message, sizeof(DataStruct));
		}
		/*
		else{
			
			msg_type = SHANNON_MESSAGE_NO_MORE_TYPE;
		}
		*/
		return this->SendMessage<DataStruct>(&msg, msg_type);
	}	
	
	////发送消息
	template<class DataStruct>
	bool SendMessage( const MessageInfo<DataStruct> * message, MessageType msg_type ){
		char buffer_send_data[sizeof(MessageType) + sizeof(MessageInfo<DataStruct>)];
		////if(!message) msg_type = SHANNON_MESSAGE_NO_MORE_TYPE;

		std::size_t transferred_size = sizeof( MessageType );
		memcpy( buffer_send_data, (void*)(&msg_type), sizeof( MessageType ) );
		if(message){
			transferred_size += sizeof(MessageInfo<DataStruct>);
			memcpy( buffer_send_data + sizeof( MessageType ), (void*)(message), sizeof(MessageInfo<DataStruct>) );
		}
		bool isSent = UnifiedSendMessage(socket_, buffer_send_data, transferred_size );

		if( ! isSent ){
			logger_( -data_received_-1, std::string("ClientBaseSync::SendMessage failed for key:") + (message ? std::string(  message->content.key_id() ) : "NULL") );
			is_connected_.store(false);
		}
		return isSent;	
	}
};


}//namespace Message
}//namespace Shannon

#endif
