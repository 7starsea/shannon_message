///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef SHANNON_MESSAGE_SERVER_NON_SEQUENCE_HPP
#define SHANNON_MESSAGE_SERVER_NON_SEQUENCE_HPP



#include <shannon/message/server_base.h>
#include <shannon/message/observer.hpp>

/*MessengerNonSequenceImpl*/
namespace Shannon{
	namespace Message{

		template<typename ReceiveStruct>
class MessengerNonSequenceImpl : public MessengerBase{
public:
	MessengerNonSequenceImpl( boost::asio::io_service & io_service, MessengerBaseContainer & container ,const std::function< void(int, const std::string &) > & logger, const std::vector< Observer<ReceiveStruct> * > & observers_map )
		:MessengerBase(io_service, container, logger), observers_map_(observers_map),recent_message_type_(SHANNON_MESSAGE_NULL_VOID_TYPE){}
	virtual ~MessengerNonSequenceImpl(){};
	virtual void Init();
	
private:
		//监听消息
	void __Listen();
		//处理消息
	void __OnListenType(const std::shared_ptr<MessengerBase> &, const boost::system::error_code& error, std::size_t bytes_transferred);
	void __OnListenData(const std::shared_ptr<MessengerBase> &, const boost::system::error_code& error, std::size_t bytes_transferred);
	void __UnRegister();
private:
	const std::vector< Observer<ReceiveStruct> * >   observers_map_;
		//最新接收到的消息类型
	MessageType recent_message_type_;
	MessageType buffer_type_;
	MessageInfo<ReceiveStruct>  buffer_data_;
};

template<typename ReceiveStruct>
void MessengerNonSequenceImpl<ReceiveStruct>::__UnRegister(){
	///注销当前连接
	for( auto it=observers_map_.begin(); it!=observers_map_.end(); ++it )	{
		(*it)->EraseMessenger( unique_id_ );
	}
}

template<typename ReceiveStruct>
void MessengerNonSequenceImpl<ReceiveStruct>::Init(){
	this->_Init();
	//注册当前连接
//	for( auto it=observers_map_.begin(); it!=observers_map_.end(); ++it ){
//		(*it)->RegisterMessenger( unique_id_, std::weak_ptr<MessengerBase>( shared_from_this() )  );
//	}
	__Listen();
}

template<typename ReceiveStruct>
void MessengerNonSequenceImpl<ReceiveStruct>::__Listen()
{
	boost::asio::async_read( *socket_.get(),
        boost::asio::buffer( (void*)(&buffer_type_), sizeof(buffer_type_) ),
            boost::bind(&MessengerNonSequenceImpl<ReceiveStruct>::__OnListenType,
                this,	 shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
};

template<typename ReceiveStruct>
void MessengerNonSequenceImpl<ReceiveStruct>::__OnListenType(const std::shared_ptr<MessengerBase> &session, const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if (error ) {
		std::string t("Messenger::OnListenType: ");
		t += boost::system::system_error(error).what();
		logger_( container_.size(), t );

	   _Remove();
		__UnRegister();
    }else{
		recent_message_type_ = buffer_type_;//*(msg_type.get());

#ifdef SHANNON_MESSAGE_SERVER_DEBUG
		logger_( container_.size(), "Messenger::OnListenType Recent Message Type:"+boost::lexical_cast<std::string>( recent_message_type_ ) );
#endif
		if( SHANNON_MESSAGE_NO_MORE_TYPE == recent_message_type_ ){
			for( auto it=observers_map_.begin(); it!=observers_map_.end(); ++it )	{
				(*it)->CleanMessenger( unique_id_ );
			}
			this->__Listen();	
		}else{
			//std::shared_ptr<MessageInfo<ReceiveStruct> > msg(new MessageInfo<ReceiveStruct> );
			boost::asio::async_read( *socket_.get(),
					boost::asio::buffer( (void*)(&buffer_data_), sizeof(buffer_data_) ),
				boost::bind(&MessengerNonSequenceImpl<ReceiveStruct>::__OnListenData,
					this, session,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
	}
}
template<typename ReceiveStruct>
void MessengerNonSequenceImpl<ReceiveStruct>::__OnListenData(const std::shared_ptr<MessengerBase> &session, const boost::system::error_code& error, std::size_t bytes_transferred)
{
	bool isValid = false;
	if (error ) {
		std::string t("Messenger::OnListenData: ");
		t += boost::system::system_error(error).what();
		logger_( container_.size(), t );
    }else {
		if( SHANNON_MESSAGE_CHECK_FIELD == buffer_data_.check_field ){
#ifdef SHANNON_MESSAGE_SERVER_DEBUG
		logger_( container_.size(), "Messenger::OnListenData MESSAGE_CHECK_FIELD Passed with Observer Size:"+boost::lexical_cast<std::string>( observers_map_.size() ) );
#endif
			ReceiveStruct * pSubField = &(buffer_data_.content);
			for( auto it=observers_map_.begin(); it!=observers_map_.end(); ++it ){
				(*it)->OnReceivedMsg( unique_id_,std::weak_ptr<MessengerBase>(session), pSubField, recent_message_type_ );
			}
			isValid = true;
		}else{
			std::string t("Can not Match Message check field, Please check datastruct consistency! Corresponding Datastruct is: ");
			t += typeid(ReceiveStruct).name();
			logger_( container_.size(), t );
		}
	}
	if( !isValid ){
		_Remove();
		__UnRegister();
		return;
	}
	this->__Listen();	
}





/*Server*/
template<typename ReceiveStruct>
class ServerNonSequenceImpl  : public ServerBase
{
public:
    ServerNonSequenceImpl( boost::asio::ip::tcp::endpoint &endpoint, const std::vector< Observer<ReceiveStruct> * > & observers_map, 
		const std::function< void(int, const std::string &) > logger = UnifiedLoggerForServer  )
		:ServerBase(endpoint, logger), observers_map_(observers_map){}
	ServerNonSequenceImpl( unsigned short port, const std::vector< Observer<ReceiveStruct> * > & observers_map, 
		const std::function< void(int, const std::string &) > logger = UnifiedLoggerForServer )
		:ServerBase(port, logger), observers_map_(observers_map){}
	virtual ~ServerNonSequenceImpl(){}
protected:
	//监听客户连接
	virtual void _Listen();
	//处理客户连接
	void _OnListen(const std::shared_ptr<MessengerNonSequenceImpl<ReceiveStruct> > & new_session, const boost::system::error_code& error );

private:
	const std::vector< Observer<ReceiveStruct> * >  observers_map_;
};



template<typename ReceiveStruct>
void ServerNonSequenceImpl<ReceiveStruct>::_Listen()
{
    std::shared_ptr<MessengerNonSequenceImpl<ReceiveStruct> > new_session(new MessengerNonSequenceImpl<ReceiveStruct>( io_service_,container_, logger_, observers_map_  ));
	try{
		acceptor_.async_accept( *(new_session->socket().get()),
			boost::bind(&ServerNonSequenceImpl<ReceiveStruct>::_OnListen,
				this,
				new_session,
				boost::asio::placeholders::error));
	}catch(...)	{
		logger_( container_.size(), "Unexpected exception caught in Server Listen: "+  boost::current_exception_diagnostic_information() );
	}
}

template<typename ReceiveStruct>
void ServerNonSequenceImpl<ReceiveStruct>::_OnListen(const std::shared_ptr<MessengerNonSequenceImpl<ReceiveStruct> > & new_session, const boost::system::error_code& error) {
    if (error) {
		logger_( container_.size(), std::string("Server::OnListen  Stopped For Listening for client! error: ") + error.message() );
		
		stopped_.store(true);
		io_service_.stop();
		return;  
	}
	
	if( !stopped_.load() ){
		_Listen();	
		if( container_.size() >= 3 * running_threads_.load(boost::memory_order_consume) ){
			std::thread t( std::bind(&ServerNonSequenceImpl<ReceiveStruct>::_enter_run_loop, this, false ) );
			t.detach();
		}
		new_session->Init();
	}
};


}//namespace Message
}//namespace Shannon

#endif