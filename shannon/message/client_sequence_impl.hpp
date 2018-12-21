///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef SHANNON_MESSAGE_CLIENT_SEQUENCE_IMPL_HPP
#define SHANNON_MESSAGE_CLIENT_SEQUENCE_IMPL_HPP
#include <functional>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/find.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/transform.hpp>

#include <boost/lexical_cast.hpp> 
#include "client_base.h"

namespace Shannon{
	namespace Message{

/*
Current Requirement: SendStruct should have public member variables: id, resume_type; and all other possible data will set to 0;; 
Application: for ConsoleSubscribeField: is_feedback will be set false
			 for SubscribeField		  : keep_order will be set false
*/
template<typename VecDataStruct>
class ClientSequenceImpl : public ClientBaseSync{
private:

	struct _InstanceBase{
		const MessageTypeGroup  message_group;
		const int size;
		const int index;

		explicit _InstanceBase(const MessageTypeGroup & t_group,
					  const int t_size,
					  const int t_index
					  ):message_group(t_group), size(t_size), index(t_index){}

		virtual ~_InstanceBase(){}
	
	};
	template<typename DataStruct>
	struct _Instance : public _InstanceBase{
		typedef DataStruct type;
		const std::function< void(DataStruct*)> updater;	

		explicit _Instance(const MessageTypeGroup & t_group,
					  const std::function< void(DataStruct*)> t_updater,
					  const int t_index)
					  :_InstanceBase(t_group, (int)sizeof( MessageInfo<DataStruct> ), t_index), updater(t_updater){}

		virtual ~_Instance(){}
	};

	struct for_each_wrapper_update{
		for_each_wrapper_update(ClientSequenceImpl<VecDataStruct> * orig_client,  _InstanceBase * instance, char * message_content)
			:orig_client_(orig_client), instance_(instance), message_content_(message_content){}
		template<typename T>
		void operator()(T ){

			if( T::index::value == instance_->index ){
#ifdef SHANNON_MESSAGE_CLIENT_DEBUG
				orig_client_->logger_(orig_client_->data_received_+1, std::string("Update Find Data Type:") + typeid(typename T::type).name()  );	
#endif
				MessageInfo<typename T::type> * msg = ( MessageInfo<typename T::type> * )( message_content_ );
				_Instance<typename T::type> * tt = static_cast< _Instance<typename T::type>* >( instance_ );
				if( SHANNON_MESSAGE_CHECK_FIELD ==  msg->check_field ){
					tt->updater( &(msg->content) );
				}else{
					std::string t_msg("Can not Match Message check field with server: ");
					t_msg += orig_client_->listen_addr_; t_msg += ", please check datastruct consistency! Corresponding Datastruct is: ";
					t_msg += typeid(typename T::type).name();
					orig_client_->logger_(- orig_client_->data_received_-1, t_msg);
					
					orig_client_->auto_reconnected_.store(false);
					
					orig_client_->_ReConnect();
				}
			}
		}

	private:
		ClientSequenceImpl<VecDataStruct> * orig_client_;
		_InstanceBase * instance_;
		char * message_content_;
	};
public:

	ClientSequenceImpl(const std::string & listen_addr, 
						const std::function< void(int, const std::string &) > logger = &UnifiedLoggerForClient)
						:ClientBaseSync(listen_addr, logger),
						recent_message_type_( SHANNON_MESSAGE_NULL_VOID_TYPE ){}

	virtual ~ClientSequenceImpl();

	template<typename DataStruct>
	void Register( const MessageTypeGroup & t_group, const std::function< void(DataStruct*)>& t_updater);

protected:
	inline virtual void _Listen(){
		/////ListenType
		_ListenImpl(boost::mpl::false_());
	}
private:

	void _ListenImpl(boost::mpl::false_);///Listen Type
	void _ListenImpl(boost::mpl::true_);//Listen Data
	//实现异步的对监听到的消息的处理
	void _OnListenImplType(const std::shared_ptr<ClientBase> &, const boost::system::error_code& error, std::size_t bytes_transferred );
	void _OnListenImplData(const std::shared_ptr<ClientBase> &, const boost::system::error_code& error, std::size_t bytes_transferred );
private:
	typedef typename boost::mpl::transform<VecDataStruct, AddTypeWrapperHelper<VecDataStruct> >::type vec_data_struct_wrap_;
	MessageType recent_message_type_;

	char  buffer_data_[sizeof(MessageInfo<typename MaxSizeTypeHelper<VecDataStruct>::type>)];

	std::map<MessageType, _InstanceBase* > instances_;
	std::vector< int > instance_type_indexes_;
};

template<typename VecDataStruct>
ClientSequenceImpl<VecDataStruct>::~ClientSequenceImpl(){
	for(auto it = instances_.begin(); it != instances_.end(); ++it ){
		delete it->second;
		it->second = NULL;
	}
}


template<typename VecDataStruct>
template<typename DataStruct>
void ClientSequenceImpl<VecDataStruct>::Register(const MessageTypeGroup & t_group,const std::function< void(DataStruct*)> & t_updater)
{
	typedef typename boost::mpl::find<VecDataStruct, DataStruct>::type found_type;
	BOOST_STATIC_ASSERT_MSG( boost::is_same<typename boost::mpl::deref<found_type>::type, DataStruct >::value, "Invalid/Incompatible DataStruct" );
	///BOOST_MPL_ASSERT((boost::is_same<typename boost::mpl::deref<found_type>::type, DataStruct > ));
	
	auto it = std::find(instance_type_indexes_.begin(), instance_type_indexes_.end(), found_type::pos::value);
	if( it == instance_type_indexes_.end() && instances_.find( t_group.update ) == instances_.end() ){
		_Instance<DataStruct> * t = new _Instance<DataStruct>(t_group, t_updater, found_type::pos::value);
		instances_.insert( std::pair<MessageType, _InstanceBase*>(  t_group.update , t ) );
		instance_type_indexes_.push_back( found_type::pos::value );
	}else{
		std::string t( "You have already registered the datastruct: " );
		t += typeid(DataStruct).name();
		t += " or the message update type: ";
		t += boost::lexical_cast<std::string>( t_group.update );
		logger_(-data_received_-1, t );
	}
}



//Listen Type
template<typename VecDataStruct>
void ClientSequenceImpl<VecDataStruct>::_ListenImpl(boost::mpl::false_){
	boost::asio::async_read(*(socket_.get()),
                boost::asio::buffer( (void*)(&buffer_type_), sizeof(buffer_type_ ) ),
                boost::bind(&ClientSequenceImpl<VecDataStruct>::_OnListenImplType,
                this,shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
}
//Listen Data
template<typename VecDataStruct>
void ClientSequenceImpl<VecDataStruct>::_ListenImpl(boost::mpl::true_){
	auto it = instances_.find( recent_message_type_ );
	if( it != instances_.end() ){
			memset(buffer_data_, 0, sizeof(buffer_data_));
			boost::asio::async_read(*(socket_.get()),
				boost::asio::buffer(buffer_data_, it->second->size),
				boost::bind(&ClientSequenceImpl<VecDataStruct>::_OnListenImplData,
				this, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}else{
		logger_( -data_received_-1, std::string( "Unexpected Message Type:" ) + boost::lexical_cast<std::string>( recent_message_type_ ) );
		this->_ReConnect();	
	}
}



	//实现异步的对监听到的消息的处理
template<typename VecDataStruct>
void ClientSequenceImpl<VecDataStruct>::_OnListenImplType(const std::shared_ptr<ClientBase> &, const boost::system::error_code& error, std::size_t bytes_transferred ){
	G_UNUSED(bytes_transferred);
	if( error ){
		logger_(-data_received_-1, " Messenger::OnListenType Failed. Disconnected from the server!" );
		this->_ReConnect();
		return;
	}
	recent_message_type_ = buffer_type_;
	switch (recent_message_type_)
	{
	case SHANNON_MESSAGE_ACTIVE_CLOSE_TYPE:
		logger_(-data_received_-1, std::string("Received SHANNON_MESSAGE_ACTIVE_CLOSE_TYPE!"));
		this->_ReConnect();///active close
		break;
	case SHANNON_MESSAGE_HEART_BEATING_TYPE:
#ifdef SHANNON_MESSAGE_CLIENT_DEBUG
		logger_(data_received_+1, std::string("Received SHANNON_MESSAGE_HEART_BEATING_TYPE!"));
#endif
		_ListenImpl(boost::mpl::false_());////Listen Type
		break;
	case SHANNON_MESSAGE_NULL_VOID_TYPE:
	case SHANNON_MESSAGE_NO_MORE_TYPE:
#ifdef SHANNON_MESSAGE_CLIENT_DEBUG
		logger_(data_received_+1, std::string("Received SHANNON_MESSAGE_NULL_VOID_TYPE|SHANNON_MESSAGE_NO_MORE_TYPE!"));
#endif
		_ListenImpl(boost::mpl::false_());////Listen Type
		break;
	default:
#ifdef SHANNON_MESSAGE_CLIENT_DEBUG
		logger_(data_received_+1, std::string("Received Message Type:") + boost::lexical_cast<std::string>( recent_message_type_ )  );
#endif
		_ListenImpl(boost::mpl::true_());//listen data
		break;
	}
	
}

template<typename VecDataStruct>
void ClientSequenceImpl<VecDataStruct>::_OnListenImplData(const std::shared_ptr<ClientBase> &, const boost::system::error_code& error, std::size_t bytes_transferred ){
	G_UNUSED(bytes_transferred);
	if( error ){
		logger_(-data_received_-1, "Messenger::OnListenData Failed. Disconnected from the server!" );
		this->_ReConnect();
		return;
	}	

	auto it = instances_.find( recent_message_type_ );
	if( it != instances_.end() ){
		++data_received_;
#ifdef SHANNON_MESSAGE_CLIENT_DEBUG
		logger_(data_received_+1, std::string("OnListenData Instance Update Type:") + boost::lexical_cast<std::string>( it->second->message_group.update )  );
#endif
		boost::mpl::for_each< vec_data_struct_wrap_ >( for_each_wrapper_update(this, it->second, buffer_data_)  );
	}

#ifdef SHANNON_MESSAGE_CLIENT_DEBUG
	if( it == instances_.end() ){
		logger_(data_received_+1, std::string("OnListenData Invalid Message Type:") + boost::lexical_cast<std::string>( recent_message_type_ )  );	
	}
#endif

	_ListenImpl(boost::mpl::false_());////Listen Type
}



}//namespace Message
}//namespace Shannon


#endif