///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef SHANNON_MESSAGE_CLIENT_NON_SEQUENCE_IMPL_HPP
#define SHANNON_MESSAGE_CLIENT_NON_SEQUENCE_IMPL_HPP
#include <functional>
#include <boost/lexical_cast.hpp> 
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/is_sequence.hpp>

#include <shannon/message/client_base.h>
namespace Shannon{
	namespace Message{

/*
Current Requirement: SendStruct should have public member variables: id, resume_type; and all other possible data will set to 0;; 
Application: for ConsoleSubscribeField: is_feedback will be set false
			 for SubscribeField		  : keep_order will be set false

May Receive Data with key not in the listen_keys and will not filter these keys for speed reason
*/
template<class ReceiveStruct>
class ClientNonSequenceImpl : public ClientBase{
public:
	////继承自ClientBase类，加入了对收到的消息更新的处理函数，可通过初始化时绑定具体的updater函数来实现具体的更新处理方法	
	ClientNonSequenceImpl(const std::string & listen_addr,  
			 const std::function< void(int, const std::string &) > logger = &UnifiedLoggerForClient
			 ):ClientBase(listen_addr,   logger),  recent_message_type_(SHANNON_MESSAGE_NULL_VOID_TYPE){}

	template<typename DataStruct>
	inline void Register( const MessageTypeGroup & t_group, const std::function< void(DataStruct*)>& t_updater){

		BOOST_MPL_ASSERT(( boost::is_same<DataStruct, ReceiveStruct> ));
		BOOST_MPL_ASSERT_NOT(( boost::is_same<void, ReceiveStruct> ));

		message_group_ = t_group;
		updater_ = t_updater;
	}
protected:
	inline virtual void _Listen(){
		/////ListenType
		_ListenImpl(boost::mpl::false_());
	}
private:
	void _ListenImpl(boost::mpl::false_);///Type
	void _ListenImpl(boost::mpl::true_);//Data
	//实现异步的对监听到的消息的处理
	void _OnListenImplType(const std::shared_ptr<ClientBase> &, const boost::system::error_code& error, std::size_t bytes_transferred );
	void _OnListenImplData(const std::shared_ptr<ClientBase> &, const boost::system::error_code& error, std::size_t bytes_transferred );
private:
	//最新接收到的消息类型
	MessageType recent_message_type_;
	//该messager_base对象交互的消息类型组
	MessageTypeGroup message_group_;
	//接收消息回调函数
	std::function< void(ReceiveStruct *)> updater_;	
	
	typedef typename boost::mpl::if_< boost::is_same<void, ReceiveStruct>, MessageType, ReceiveStruct >::type buffer_data_type_;
	MessageInfo<buffer_data_type_>	buffer_data_;
	MessageType	buffer_type_;
};



/**
template<class ReceiveStruct>
void ClientNonSequenceImpl<ReceiveStruct, SendStruct>::__Listen( boost::mpl::true_ ){

	typedef typename boost::mpl::front<SendStruct>::type SubscribeType;
	
	MessageInfo<SubscribeType> subscribe_struct;

	memset(&(subscribe_struct.content), 0, sizeof(subscribe_struct.content));
	subscribe_struct.content.resume_type = resume_type_;

	int succ=0;
	for(auto itKey = listen_keys_.begin(); itKey != listen_keys_.end(); ++itKey ){
		strncpy(subscribe_struct.content.id, itKey->c_str(), sizeof(subscribe_struct.content.id) - 1);
		if( this->_SendMessage<SubscribeType>( &subscribe_struct, message_group_.subscribe ) ) ++succ;
		else break;
	}
	std::string strmsg = "Sent: "; 
	strmsg += boost::lexical_cast<std::string>( succ ) ; strmsg += " Succeed; ";
	strmsg += boost::lexical_cast<std::string>( listen_keys_.size() ) ; strmsg += " Total; ";

	logger_(1, strmsg );
	resume_type_ = SHANNON_MESSAGE_RESUME_QUICK;  ////之后重新订阅的话使用Quick Mode

		////Send SHANNON_MESSAGE_NO_MORE_TYPE
	this->_SendMessage<SubscribeType>( NULL, SHANNON_MESSAGE_NO_MORE_TYPE );
	/////ListenType
	_ListenImpl( boost::mpl::false_() );
}
template<class ReceiveStruct>
void ClientNonSequenceImpl<ReceiveStruct, SendStruct>::__Listen( boost::mpl::false_ ){
    /////ListenType
	_ListenImpl(boost::mpl::false_());
}
**/


////////ListenType
template<class ReceiveStruct>
void ClientNonSequenceImpl<ReceiveStruct>::_ListenImpl(boost::mpl::false_){
	boost::asio::async_read(*(socket_.get()),
                boost::asio::buffer( (void*)(&buffer_type_), sizeof(buffer_type_ ) ),
                boost::bind(&ClientNonSequenceImpl<ReceiveStruct>::_OnListenImplType,
                this, shared_from_this(),  boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
}
////////ListenData
template<class ReceiveStruct>
void ClientNonSequenceImpl<ReceiveStruct>::_ListenImpl(boost::mpl::true_){
	boost::asio::async_read(*(socket_.get()),
            boost::asio::buffer( (void*)(&buffer_data_), sizeof(buffer_data_) ),
            boost::bind(&ClientNonSequenceImpl<ReceiveStruct>::_OnListenImplData,
            this, shared_from_this(),  boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}
template<class ReceiveStruct>
void ClientNonSequenceImpl<ReceiveStruct>::_OnListenImplType(const std::shared_ptr<ClientBase> &, const boost::system::error_code& error, std::size_t bytes_transferred )
{
	if( error ){
        logger_(-data_received_-1,std::string("Messenger::OnListenType Failed. Disconnected from the server! Error:") + error.message() );
		this->_ReConnect();
		return;
	}
	recent_message_type_ = buffer_type_;
	if( SHANNON_MESSAGE_ACTIVE_CLOSE_TYPE == recent_message_type_ ){
		logger_(-data_received_-1, std::string("Received SHANNON_MESSAGE_ACTIVE_CLOSE_TYPE!"));
		this->_ReConnect(  );
	}else{
		typedef typename boost::mpl::if_< boost::is_same<void, ReceiveStruct>  ,
														boost::mpl::false_,
														boost::mpl::true_
									  >::type true_false_type;
           _ListenImpl(  true_false_type() );
	}
}
template<class ReceiveStruct>
void ClientNonSequenceImpl<ReceiveStruct>::_OnListenImplData(const std::shared_ptr<ClientBase> &, const boost::system::error_code& error, std::size_t bytes_transferred )
{
	BOOST_MPL_ASSERT_NOT(( boost::is_same<void, ReceiveStruct> ));
	if( error ){
        logger_(-data_received_-1, std::string("Messenger::OnListenData Failed. Disconnected from the server!") + error.message() );
		this->_ReConnect();
		return;
	}
#ifdef SHANNON_MESSAGE_CLIENT_DEBUG
		logger_(data_received_+1, std::string("OnListenData Recent Message Type:") 
											+ boost::lexical_cast<std::string>( recent_message_type_ )
											+ " Update Message Type: " + boost::lexical_cast<std::string>( message_group_.update )
											);
#endif
	if( message_group_.update == recent_message_type_ ){
		if( SHANNON_MESSAGE_CHECK_FIELD == buffer_data_.check_field ){
			++data_received_;
			if(updater_)updater_( &(buffer_data_.content) );
#ifdef SHANNON_MESSAGE_CLIENT_DEBUG
			if( !updater_ )
				logger_(data_received_+1, std::string("OnListenData No Updater") );
#endif			
			_ListenImpl( boost::mpl::false_() );
		}else{
			std::string t_msg("Can not Match Message check field with server: ");
			t_msg += listen_addr_; t_msg += ", please check datastruct consistency! Corresponding Datastruct is: ";
			t_msg += typeid(ReceiveStruct).name();
			logger_(-data_received_-1, t_msg );

			auto_reconnected_.store(false);
			this->_ReConnect();			
		}
	}
}




}//namespace Message
}//namespace Shannon


#endif
