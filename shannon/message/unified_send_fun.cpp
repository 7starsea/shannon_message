///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#include "unified_send_fun.h"
#include <boost/exception/diagnostic_information.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
namespace Shannon{
	namespace Message{

bool UnifiedSendMessage(const std::shared_ptr<boost::asio::ip::tcp::socket> & tcp_socket, const void * msg, const std::size_t bytes_need_transferred,  int retry_interval_in_millisec /*= 50*/)
{
	bool isSent = false;
	try{
		boost::system::error_code error;
		std::size_t bytes_transferred = 0, current_transferred=0;
		boost::asio::const_buffers_1  b1 = boost::asio::buffer( msg, bytes_need_transferred );
		unsigned int num_tried = 0; int resume = 1;
		while( num_tried < bytes_need_transferred - bytes_transferred && tcp_socket->is_open() && num_tried <=10){
			boost::asio::const_buffers_1  b2 = boost::asio::buffer( b1 + bytes_transferred,  bytes_need_transferred - bytes_transferred);
			current_transferred = boost::asio::write( *tcp_socket.get(), b2, error);
			bytes_transferred += current_transferred ;
			if( error == boost::asio::error::would_block || error == boost::asio::error::try_again || bytes_transferred < bytes_need_transferred){
				++num_tried;
				if( current_transferred ){
					if( current_transferred * 10 < bytes_need_transferred - bytes_transferred )
						resume += 1;
					else
						resume = 1;
					if( resume > 6 ) resume = 6;
				}else{
					if( resume > 8 ){
						////wait too long, cancel this data transferred
						//num_tried = bytes_need_transferred + 1; 
						break;
					}
					if( resume < 7 ) resume = 7;
					else resume += 1;
				}
				UnifiedSleep( resume * retry_interval_in_millisec );
			}else{
				num_tried = bytes_need_transferred + 1;
			}
		}
		if( error || bytes_transferred != bytes_need_transferred )	{
			std::cerr<<"===> UnifiedSendMessage failed! Error: "<<error.message()<<". Transferred: "<<bytes_transferred<<"/"<<bytes_need_transferred<<std::endl;
		}else{
			isSent = true;
		}	
	}catch(...)	{
		std::cerr <<  "===> UnifiedSendMessage failed! Unexpected Error: "
				  << boost::current_exception_diagnostic_information()<< std::endl; 
	}	

	if( !isSent ){	
		boost::system::error_code ec;
		tcp_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		tcp_socket->close(ec);

	}

	return isSent;
};

void UnifiedLoggerForClient(int data_received, const std::string & msg){
	std::string timestamp=	boost::posix_time::to_simple_string(boost::posix_time::second_clock::local_time());
	std::cerr<<"===> Client Time: "<<timestamp<<" Data Received: "<<data_received
			<<"\n     Message: "<<msg<<"\n";
}
void UnifiedLoggerForServer(int clients, const std::string & msg){
	std::string timestamp= boost::posix_time::to_simple_string(boost::posix_time::second_clock::local_time());
	std::cerr<<"===> Server Time: "<<timestamp<<" Total Clients: "<<clients
			<<"\n     Message: "<<msg<<"\n";
}



}//namespace Message
}//namespace Shannon
