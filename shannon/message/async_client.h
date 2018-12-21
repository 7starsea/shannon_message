///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef SHANNON_MESSAGE_ASYNC_CLIENT_H
#define SHANNON_MESSAGE_ASYNC_CLIENT_H

#include <iostream>
#include <string>
#include <memory>
#include <mutex>
#include <functional>
#include <boost/asio.hpp>
#include <boost/predef.h>

#include <shannon/message/client_base.h>

namespace Shannon{
	namespace Message{


#define SHANNON_MESSAGE_MAX_ASYNC_BUFFER_SIZE 16384
		////we could also use std::mutex
		typedef SpinLockMutex MyGenericMutex;

		class UnifiedAsyncSender: public std::enable_shared_from_this<UnifiedAsyncSender>{
		public:
			UnifiedAsyncSender(const std::shared_ptr<boost::asio::ip::tcp::socket> & tcp_socket);

			~UnifiedAsyncSender(){
			//	std::cout<<"Async Write:"<<write_count_<<std::endl;
			}

			void set_error_handler(const std::function< void(const boost::system::error_code&) > & handler_writer){
				handler_writer_ = handler_writer;
			}

			bool reset();

			bool async_send( MessageType msg_type, const void * message, const std::size_t message_size, bool sentImmediately=true);
			
			bool async_send(const void * msg, 	const std::size_t bytes_need_transferred, bool sentImmediately=true){
				std::lock_guard<MyGenericMutex> guard(mutex_);
				return _async_send(msg, bytes_need_transferred, sentImmediately);
			}
		protected:
			void _switch_buffer();
			bool _async_send(const void * msg, 	const std::size_t bytes_need_transferred, bool sentImmediately);
		protected:
			const std::shared_ptr<boost::asio::ip::tcp::socket> & tcp_socket_;
			std::function< void(const boost::system::error_code&) > handler_writer_;
			
			/*! Buffer structure 
			   [occupied buffer][available_buffer][sending buffer][occupied buffer][available_buffer] 
			 */
			char buffer1_[SHANNON_MESSAGE_MAX_ASYNC_BUFFER_SIZE];
			char buffer2_[SHANNON_MESSAGE_MAX_ASYNC_BUFFER_SIZE];
			char * using_buffer_;
			char * backup_buffer_;

			///all data before this point has been sent
			std::size_t sending_address_;
			///Sending buffer size; 
			std::size_t sending_size_;
			///upcoming sending message at the beginning of sending buffer
			std::size_t pre_occupied_size_; ///buffer message size/starting at the beginning/for using buffer
			///upcoming sending message at the end of sending buffer
			std::size_t post_occupied_size_;  ///buffer message size/starting at the sending_address+sending_size
			
			std::size_t backup_occupied_size_;

		///	std::size_t write_count_;
			
			MyGenericMutex mutex_;

			char * __RtnBuffer(const std::size_t bytes_need_transferred);
			void __InternalAsyncSend();
			void OnInternalAsyncSend(const boost::system::error_code& error, std::size_t bytes_transferred);
		};


#undef SHANNON_MESSAGE_MAX_ASYNC_BUFFER_SIZE








		class ClientBaseAsync : public ClientBase{
		public:
			///构造函数，需要给出Server的地址，消息类型组，是否不间断打印日志和Logger函数
			ClientBaseAsync(const std::string & listen_addr,  /*地址格式，例如: 127.0.0.1:8000*/
							const std::function< void(int, const std::string &) > logger = &UnifiedLoggerForClient 
							):ClientBase(listen_addr, logger),async_sender_(new UnifiedAsyncSender(socket_)){}

			virtual ~ClientBaseAsync(){		}
	
			inline virtual void _Listen(){
				async_sender_->set_error_handler(std::bind(&ClientBaseAsync::OnErrorAsyncSend, this, shared_from_this(), std::placeholders::_1) );

				async_sender_->reset();
				ClientBase::_Listen();
			}

			void OnErrorAsyncSend(const std::shared_ptr<ClientBase> &,  const boost::system::error_code& error){		
				logger_(-data_received_-1, " ClientBase::OnErrorAsyncSend Failed. Disconnected from the server!" + std::string(error.message()) );
				this->_ReConnect();
			}


			bool SendMessage( MessageType msg_type ){
				////message type should not be greater than zero
				bool isSent =  async_sender_->async_send((void *)&msg_type, sizeof(MessageType)); 
				if( ! isSent ){
					logger_( data_received_+1, std::string("ClientBaseAsync::SendMessage buffer not enough for message type.") );
				}
				return isSent;	
			}

			////发送消息
			template<class DataStruct>
			bool SendMessage( DataStruct * message, MessageType msg_type ){
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
			bool SendMessage( MessageInfo<DataStruct> * message, MessageType msg_type ){
				bool isSent = false;
				if(is_connected()){
					if(message){
						isSent = async_sender_->async_send(msg_type, (void *)message, sizeof(MessageInfo<DataStruct>));
						if(!isSent){
							UnifiedSleep(30); ///wait for 50 milli seconds
		//					count_sleep_.fetch_add(1, boost::memory_order::memory_order_relaxed);
							isSent = async_sender_->async_send(msg_type, (void *)message, sizeof(MessageInfo<DataStruct>));
						}
					}else{
						////msg_type = SHANNON_MESSAGE_NO_MORE_TYPE;
						isSent = async_sender_->async_send((void *)&msg_type, sizeof(MessageType)); 
					}

					if( ! isSent ){
						logger_( data_received_+1, std::string("ClientBaseAsync::SendMessage buffer not enough for key:") + (message ? std::string(  message->content.key_id() ) : "NULL") );
					}
				}
				return isSent;	
			}

		protected:
			const std::shared_ptr<UnifiedAsyncSender> async_sender_;
		//	boost::atomic<int> count_sleep_ ;
		};














}//namespace Message
}//namespace Shannon
#endif
