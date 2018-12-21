///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#include "async_client.h"
#include "unified_send_fun.h"
#include <boost/exception/diagnostic_information.hpp>


#include <boost/bind.hpp>
#include <algorithm>

namespace Shannon{
	namespace Message{


		///async sender
		UnifiedAsyncSender::UnifiedAsyncSender(const std::shared_ptr<boost::asio::ip::tcp::socket> & tcp_socket)
								: tcp_socket_(tcp_socket),handler_writer_(),

								using_buffer_(NULL), backup_buffer_(NULL),
								sending_address_(0), sending_size_(0), pre_occupied_size_(0), post_occupied_size_(0),
								backup_occupied_size_(0),
							///	write_count_(0),
								mutex_()
		{
			using_buffer_ = (char*)buffer1_;  backup_buffer_ = (char*)buffer2_;
		}
		bool UnifiedAsyncSender::reset(){
			int i = 0;
			while(i<100){
				{
					std::lock_guard<MyGenericMutex> guard(mutex_);
					if(0 == sending_size_){
						sending_address_ = 0; sending_size_ = 0; pre_occupied_size_ = 0; post_occupied_size_ = 0;
						backup_occupied_size_ = 0;
						break;
					}
				}
				UnifiedSleep(50);
				++i;
			}
			return i<100;
		}

		bool UnifiedAsyncSender::async_send( MessageType msg_type, const void * message, const std::size_t message_size, bool sentImmediately){
			std::lock_guard<MyGenericMutex> guard(mutex_);

			char * buffer = __RtnBuffer(sizeof(MessageType) + message_size);
			if(buffer){
				memcpy(buffer , (void *)&msg_type, sizeof(MessageType));
				memcpy(buffer + sizeof(MessageType), message, message_size);
			}
			G_UNUSED(sentImmediately);
			if(0 == sending_size_){
				if(0 == post_occupied_size_) _switch_buffer();
				__InternalAsyncSend();
			}
			return NULL != buffer;
		}
		
		bool UnifiedAsyncSender::_async_send(const void * msg, 	const std::size_t bytes_need_transferred, bool sentImmediately){
			char * buffer = __RtnBuffer(bytes_need_transferred);
			if(buffer){
				memcpy(buffer, msg, bytes_need_transferred);
			}

			G_UNUSED(sentImmediately);
		//	if(0 == sending_size_ && sentImmediately){
			if(0 == sending_size_){			
				if(0 == post_occupied_size_) _switch_buffer();
				__InternalAsyncSend();
			}
			return NULL != buffer;
		}
		char * UnifiedAsyncSender::__RtnBuffer(const std::size_t bytes_need_transferred){
			char * buffer = NULL;
			std::size_t next_available = sending_address_ + sending_size_ + post_occupied_size_;
			if( 0 == backup_occupied_size_ && next_available + bytes_need_transferred < sizeof(buffer1_)){
				////Put the new message to the end of using_buffer
				buffer = using_buffer_ + next_available;
				post_occupied_size_ += bytes_need_transferred;
			}else{
				next_available = backup_occupied_size_;
				if(next_available + bytes_need_transferred < sizeof(buffer1_)){
					////put message into end of backup buffer
					buffer = backup_buffer_ + next_available;
					backup_occupied_size_ += bytes_need_transferred;
				}else if(sending_address_ - pre_occupied_size_ > bytes_need_transferred){
					////put message into front of using buffer(will become backup buffer later)
					buffer = using_buffer_ + pre_occupied_size_;
					pre_occupied_size_ += bytes_need_transferred;

				}
			}
			return buffer;
		}
		
		void UnifiedAsyncSender::__InternalAsyncSend(){
			////sending_size_ must be zero
			////always sending post_occupied buffer
			std::size_t sending_size = post_occupied_size_ > 1200 ? 1200 : post_occupied_size_;
			if(sending_size && tcp_socket_->is_open()){
				post_occupied_size_ -= sending_size;
				boost::asio::async_write(*tcp_socket_.get(),
					boost::asio::buffer(using_buffer_+sending_address_, sending_size ),
					boost::bind(&UnifiedAsyncSender::OnInternalAsyncSend,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
				///++write_count_;
				sending_size_ = sending_size;
			}
		}

		void UnifiedAsyncSender::OnInternalAsyncSend(const boost::system::error_code& error, std::size_t bytes_transferred){
			std::lock_guard<MyGenericMutex> guard(mutex_);
			sending_address_ += bytes_transferred;
			sending_size_ = 0;
			if (error ) {
				if(handler_writer_)	handler_writer_(error);
				return;
			}
			 
			 if(post_occupied_size_){
				///do send message
				 __InternalAsyncSend();
			 }else if(backup_occupied_size_){
				 
				_switch_buffer();
				///do send message
				__InternalAsyncSend();
			 }
		}
		void UnifiedAsyncSender::_switch_buffer(){
			///at least should satisfy two conditions: sending_size = 0 and post_occupied_size_ = 0;

			///switch using_buffer and backup_buffer
			if(using_buffer_ == (char*) buffer1_ ){
				backup_buffer_ = (char*)buffer1_;
				using_buffer_ = (char*)buffer2_;

			}else{
				backup_buffer_ = (char*) buffer2_;
				using_buffer_ = (char*)buffer1_;

			}
			///backup buffer to using buffer
			post_occupied_size_ = backup_occupied_size_;

			backup_occupied_size_ = pre_occupied_size_;
			pre_occupied_size_ = 0;
			sending_address_ = 0;
			sending_size_ = 0;
		}

}//namespace Message
}//namespace Shannon