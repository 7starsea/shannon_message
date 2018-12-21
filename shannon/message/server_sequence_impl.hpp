///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef SHANNON_MESSAGE_SERVER_SEQUENCE_HPP
#define SHANNON_MESSAGE_SERVER_SEQUENCE_HPP

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/transform.hpp>


#include "server_base.h"
#include "observer.hpp"


namespace Shannon{
	namespace Message{

		template<typename VecDataStruct>
		class ServerSequenceImplHelper{
		private:
				struct _InstanceBase{
					const MessageType subscribe;
					///const int size;
					const int index;
					explicit _InstanceBase(const MessageTypeGroup & t_group,
									////const int t_size,
									const int t_index)
									:subscribe(t_group.subscribe),
									///size(t_size), 
									index(t_index){}
					virtual ~_InstanceBase(){}
				};
					
				template<typename DataStruct>
				struct _Instance : public _InstanceBase{
					typedef DataStruct type;
					Observer<DataStruct>*  observer;
					explicit _Instance( const MessageTypeGroup & t_group,
									const int t_index,
									Observer<DataStruct>*  t_observer
									)
									:_InstanceBase(t_group, /*(int)sizeof( MessageInfo<DataStruct> ),*/ t_index),observer(t_observer){}

					virtual ~_Instance(){}
				};

				struct for_each_clean_messenger{
					for_each_clean_messenger(ServerSequenceImplHelper<VecDataStruct> * my_helper, int unique_id)
						:my_helper_(my_helper), unique_id_(unique_id) {}

					template<typename T>
					void operator()(T ){
						for(auto it = my_helper_->instances_.begin(); it != my_helper_->instances_.end(); ++it){
							if( (*it)->index == T::index::value ){
								_Instance<typename T::type> * tobserver = static_cast< _Instance<typename T::type>* >( *it );
								tobserver->observer->CleanMessenger(unique_id_);
							}
						}
					}
				private:
					ServerSequenceImplHelper<VecDataStruct> * my_helper_;
					int unique_id_;
				};

				struct for_each_erase_messenger{
					for_each_erase_messenger(ServerSequenceImplHelper<VecDataStruct> * my_helper, int unique_id)
						:my_helper_(my_helper), unique_id_(unique_id) {}

					template<typename T>
					void operator()(T ){
						for(auto it = my_helper_->instances_.begin(); it != my_helper_->instances_.end(); ++it){
							if( (*it)->index == T::index::value ){
								_Instance<typename T::type> * tobserver = static_cast< _Instance<typename T::type>* >( *it );
								tobserver->observer->EraseMessenger(unique_id_);
							}
						}
					}
				private:
					ServerSequenceImplHelper<VecDataStruct> * my_helper_;
					int unique_id_;
				};

				struct for_each_on_rece_msg{
					for_each_on_rece_msg( ServerSequenceImplHelper<VecDataStruct> * my_helper,int unique_id,const std::weak_ptr<MessengerBase> & pMessenger, const MessageType msgType, char * content)
						: error_msg_("NOT FOUND INSTANCE!"), my_helper_(my_helper), unique_id_(unique_id),messenger_(pMessenger), msg_type_(msgType), content_(content) {}

					template<typename T>
					void operator()(T ){
						typedef typename T::type DataStruct;
						for(auto it = my_helper_->instances_.begin(); it != my_helper_->instances_.end(); ++it){
							if( (*it)->index == T::index::value && msg_type_ == (*it)->subscribe ){

								MessageInfo<DataStruct> * msg = ( MessageInfo<DataStruct> * )( content_ );
								if( SHANNON_MESSAGE_CHECK_FIELD ==  msg->check_field ){
									_Instance<DataStruct> * tobserver = static_cast< _Instance<DataStruct>* >( *it );
									tobserver->observer->OnReceivedMsg(unique_id_,messenger_, &(msg->content), msg_type_);
									error_msg_ = "";
								}else{
									error_msg_ = ("Can not Match Message check field, Please check datastruct consistency! Corresponding Datastruct is: ");
									error_msg_ += typeid(DataStruct).name();
								}

							}
						}
						
					}
				public:
					std::string error_msg_;
				private:
					ServerSequenceImplHelper<VecDataStruct> * my_helper_;
					int unique_id_;
					const std::weak_ptr<MessengerBase>  messenger_;
					const MessageType msg_type_;
					char * content_;
					
				};
		public:
			ServerSequenceImplHelper(){}

			~ServerSequenceImplHelper(){
				for(auto it = instances_.begin(); it != instances_.end(); ++it ){
					delete *it;
					*it= NULL;
				}
			}

			template<typename DataStruct>
			void Register(Observer<DataStruct> *);

			inline int RtnBufferSize( MessageType msgType ){
				auto it = type_size_.find( msgType );
				return it != type_size_.end() ? it->second : 0;
			}

			
			void EraseMessenger(  int unique_id ){
				boost::mpl::for_each< vec_data_struct_wrap_ >( for_each_erase_messenger(this, unique_id)  );
			}
			void CleanMessenger(  int unique_id ){
				boost::mpl::for_each< vec_data_struct_wrap_ >( for_each_clean_messenger(this, unique_id)  );
			}
			std::string OnReceivedMsg( int unique_id, const std::weak_ptr<MessengerBase> & pMessenger, const MessageType msgType, char * content ){
				auto it = type_size_.find( msgType );
				if( it != type_size_.end() ){
					for_each_on_rece_msg for_eachMsg(this, unique_id, pMessenger, msgType, content);
					boost::mpl::for_each< vec_data_struct_wrap_ >( boost::ref( for_eachMsg )  );
					return for_eachMsg.error_msg_;
				}
				return "NOT FOUND MESSAGE TYPE!";
			}
		protected:
			typedef typename boost::mpl::transform<VecDataStruct, AddTypeWrapperHelper<VecDataStruct>>::type vec_data_struct_wrap_;
			const std::function< void(int, const std::string &) > logger_;
			std::map<MessageType, int> type_size_;
			std::vector< _InstanceBase* > instances_;
		};

		template<typename VecDataStruct>
		template<typename DataStruct>
		void ServerSequenceImplHelper<VecDataStruct>::Register(Observer<DataStruct> * my_observer){
			typedef typename boost::mpl::find<VecDataStruct, DataStruct>::type found_type;

			BOOST_STATIC_ASSERT_MSG( boost::is_same<typename boost::mpl::deref<found_type>::type, DataStruct >::value, "Invalid/Incompatible DataStruct" );

			const MessageTypeGroup & t_group = my_observer->message_group_;

			auto it = type_size_.find( t_group.subscribe );
			if( it == type_size_.end() ){
				type_size_.insert( std::pair<MessageType, int>(  t_group.subscribe , (int)sizeof( MessageInfo<DataStruct> ) ) );
			}

			_Instance<DataStruct> * t = new _Instance<DataStruct>(t_group, found_type::pos::value, my_observer);

			instances_.push_back( t );
		}



		///////处理收到的socket instance
		template<typename VecDataStruct>
		class MessengerSequenceImpl :public MessengerBase{
		public:
			MessengerSequenceImpl(boost::asio::io_service & io_service, MessengerBaseContainer & container, const std::function< void(int, const std::string &) > & logger,
									ServerSequenceImplHelper<VecDataStruct> & helper)
									:MessengerBase(io_service, container, logger), helper_(helper){}
				virtual ~MessengerSequenceImpl(){};

				inline virtual void Init(){
					this->_Init();
					//向Observer注册当前连接
					//helper_.RegisterMessenger( unique_id_, std::weak_ptr<MessengerBase>( shared_from_this() )  );
				
					__Listen();
				}
	
		private:
			///向Observer注销当前连接
			inline void __UnRegister(){
				helper_.EraseMessenger( unique_id_ );
			}
			//监听消息
			void __Listen();
			
				//处理消息
			void __OnListenType(const std::shared_ptr<MessengerBase> &, const boost::system::error_code& error, std::size_t bytes_transferred);
			void __OnListenData(const std::shared_ptr<MessengerBase> &, const boost::system::error_code& error, std::size_t bytes_transferred);
			
			
		private:
			ServerSequenceImplHelper<VecDataStruct> & helper_;
			
			//最新接收到的消息类型
			MessageType recent_message_type_;
			MessageType buffer_type_;
			char  buffer_data_[sizeof(MessageInfo<typename MaxSizeTypeHelper<VecDataStruct>::type>)];
		};

		template<typename VecDataStruct>
		void MessengerSequenceImpl<VecDataStruct>::__Listen(){
			boost::asio::async_read( *socket_.get(),
			boost::asio::buffer( (void*)(&buffer_type_), sizeof(buffer_type_) ),
				boost::bind(&MessengerSequenceImpl<VecDataStruct>::__OnListenType,
					this,	 shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		
		template<typename VecDataStruct>
		void MessengerSequenceImpl<VecDataStruct>::__OnListenType(const std::shared_ptr<MessengerBase> & session, const boost::system::error_code& error, std::size_t bytes_transferred){
			G_UNUSED(bytes_transferred);
			if (error ) {
				std::string t("MessengerSequenceImpl::OnListenType: ");
				t += boost::system::system_error(error).what();
				logger_( container_.size(), t );

			   _Remove();		__UnRegister();
			}else{
				recent_message_type_ = buffer_type_;

#ifdef SHANNON_MESSAGE_SERVER_DEBUG
				logger_( container_.size(), "MessengerSequenceImpl::OnListenType Recent Message Type:"+boost::lexical_cast<std::string>( recent_message_type_ ) );
#endif
				switch (recent_message_type_)
				{
				case SHANNON_MESSAGE_NO_MORE_TYPE:		helper_.CleanMessenger( unique_id_ );
				case SHANNON_MESSAGE_ACTIVE_CLOSE_TYPE:
				case SHANNON_MESSAGE_NULL_VOID_TYPE:
				case SHANNON_MESSAGE_HEART_BEATING_TYPE:
					this->__Listen();
					break;
				default:
					{
						const int size = helper_.RtnBufferSize( recent_message_type_ );
						if(size > 0){
							memset(buffer_data_, 0, sizeof(buffer_data_));
							boost::asio::async_read( *socket_.get(),
								boost::asio::buffer( (void*)(buffer_data_), size ),
								boost::bind(&MessengerSequenceImpl<VecDataStruct>::__OnListenData,
								this, session,
								boost::asio::placeholders::error,
								boost::asio::placeholders::bytes_transferred));
						}else{
							logger_(container_.size(), std::string( "Unexpected Message Type:" ) + boost::lexical_cast<std::string>( recent_message_type_ ) );
							_Remove();			__UnRegister();
							/////this->__Listen();
						}
					}
					break;
				}
				/**
				if( SHANNON_MESSAGE_NO_MORE_TYPE == recent_message_type_ ){
					helper_.CleanMessenger( unique_id_ );
					this->__Listen();	
				}else{}
				**/
			}
		}

		template<typename VecDataStruct>
		void MessengerSequenceImpl<VecDataStruct>::__OnListenData(const std::shared_ptr<MessengerBase> & messenger, const boost::system::error_code& error, std::size_t bytes_transferred){
			G_UNUSED(bytes_transferred);
			bool isValid = false;
			if (error ) {
				std::string t("MessengerSequenceImpl::OnListenData: ");
				t += boost::system::system_error(error).what();
				logger_( container_.size(), t );
			}else {
				const std::string msg = helper_.OnReceivedMsg(unique_id_, std::weak_ptr<MessengerBase>( messenger), recent_message_type_, buffer_data_);
				if( msg.size() ){
					logger_( container_.size(), msg );
				}else{
					isValid = true;
				}
			}
			if( isValid ){
				this->__Listen();
			}else{
				_Remove();		__UnRegister();
			}
		}


		template<typename VecDataStruct>
		class ServerSequenceImpl  : public ServerBase
		{
		public:
			ServerSequenceImpl( boost::asio::ip::tcp::endpoint &endpoint,const std::function< void(int, const std::string &) > logger = UnifiedLoggerForServer  )
				:ServerBase(endpoint, logger), server_helper_() {}
			ServerSequenceImpl( unsigned short port, const std::function< void(int, const std::string &) > logger = UnifiedLoggerForServer )
				:ServerBase(port, logger),  server_helper_(){}
			virtual ~ServerSequenceImpl(){}

			template<typename DataStruct>
			inline void Register(Observer<DataStruct> * my_observer){
				server_helper_.template Register<DataStruct>(my_observer);
			}
		protected:
			//监听客户连接
			virtual void _Listen();
			//处理客户连接
			void _OnListen(const std::shared_ptr<MessengerSequenceImpl<VecDataStruct> > & new_session, const boost::system::error_code& error );

		private:
			ServerSequenceImplHelper<VecDataStruct> server_helper_;
		};


		template<typename VecDataStruct>
		void ServerSequenceImpl<VecDataStruct>::_Listen(){
			std::shared_ptr<MessengerSequenceImpl<VecDataStruct> > new_session(new MessengerSequenceImpl<VecDataStruct>( io_service_,container_, logger_, server_helper_ ));
			try{
				acceptor_.async_accept( *(new_session->socket().get()),
					boost::bind(&ServerSequenceImpl<VecDataStruct>::_OnListen,
						this,
						new_session,
						boost::asio::placeholders::error));
			}catch(...)	{
				logger_( container_.size(), "Unexpected exception caught in Server Listen: "+  boost::current_exception_diagnostic_information() );
			}
		}

		template<typename VecDataStruct>
		void ServerSequenceImpl<VecDataStruct>::_OnListen(const std::shared_ptr<MessengerSequenceImpl<VecDataStruct> > & new_session, const boost::system::error_code& error) {
			if (error) {
				logger_( container_.size(), std::string("ServerSequenceImpl::OnListen  Stopped For Listening for client! error: ") + error.message() );
				stopped_.store(true);
				io_service_.stop();
				return;  
			}
			
			if( !stopped_.load() ){
				_Listen();	
				if( container_.size() >= 3 * running_threads_.load(boost::memory_order_consume) ){
					std::thread t( std::bind(&ServerSequenceImpl<VecDataStruct>::_enter_run_loop, this, false ) );
					t.detach();
				}
				new_session->Init();
			}
		}


}//namespace Message
}//namespace Shannon




#endif