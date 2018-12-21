///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#include "server_base.h"
#include <boost/lexical_cast.hpp>  
#if BOOST_OS_WINDOWS
#include <Mstcpip.h>
#include <Winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#endif

namespace Shannon{
	namespace Message{

int MessengerBase::sm_total_clients_ = 0;
std::map<std::string,  int> MessengerBase::sm_client_failtimes_;

MessengerBase::MessengerBase( boost::asio::io_service & io_service, MessengerBaseContainer & container, 
							 const std::function< void(int, const std::string &) > & logger )
	:unique_id_(container.unique_id()),
	socket_( new boost::asio::ip::tcp::socket(io_service) ),
	container_(container), logger_(logger)// bind_observers_count_(0),
	 //,buffer_send_data_(NULL), buffer_send_data_size_(0)
{}

MessengerBase::~MessengerBase(){
	if( remote_address_.size() ){
        if( socket_->is_open() ){
            boost::system::error_code ec;
            socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            socket_->close(ec);
        }
		--MessengerBase::sm_total_clients_;
	
		std::string t("The Old Client:"); t += remote_address_; t +="  has been destroyed!";
		logger_( MessengerBase::sm_total_clients_, t);
	}else{
		logger_(MessengerBase::sm_total_clients_, "Unconnnected socket destroyed!");
	}
}

void MessengerBase::_Remove(){
	try{
		if( socket_->is_open() ){
			boost::system::error_code ec;
			socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            socket_->close(ec);
		}
	}catch(...){
		logger_( container_.size(), "Unexpected exception caught in MessengerBase Remove!");
	}
	container_.pop( std::enable_shared_from_this<MessengerBase>::shared_from_this() );
}
void MessengerBase::_Init(){
	boost::asio::ip::tcp::no_delay opt1(true);
//	boost::asio::ip::tcp::no_delay opt1(false);
    socket_->set_option(opt1);

//	boost::asio::socket_base::linger opt2(false, 1);
//	socket_->set_option(opt2);

	socket_->non_blocking( true );

			// platform-specific switch
#if BOOST_OS_WINDOWS			
			// the timeout value
			int timeout_seconds = 3600;
			int keepalive= 1;	
			int keepidle = 300; // 如该连接在300秒(5minutes)内没有任何数据往来,则进行探测  
			int keepinterval = 3; // 探测时发包的时间间隔为5 秒  
			int keepcount = 3; // 探测尝试的次数。如果第1次探测包就收到响应了,则后2次的不再发。  
			  int32_t timeout = timeout_seconds * 1000;
			  struct tcp_keepalive tk;
			  tk.onoff = 1;
			  tk.keepaliveinterval = keepinterval * 1000;
			  tk.keepalivetime = 300 * 1000;
			  int iResult = setsockopt(socket_->native_handle(),SOL_SOCKET,SO_KEEPALIVE, (const char*)(&keepalive),sizeof(int));
		 
			  DWORD lpcbBytesReturned = 0;
			  WSAOVERLAPPED lpOverlapped;
			  WSAIoctl(  socket_->native_handle(), SIO_KEEPALIVE_VALS, (void*)(&tk), sizeof(tk), NULL, 0, &lpcbBytesReturned, &lpOverlapped, NULL);

//			  setsockopt(socket_->native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
//			  setsockopt(socket_->native_handle(), SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#elif BOOST_OS_MACOS
			int timeout_seconds = 3600;
			int keepalive= 1;	
			setsockopt(socket_->native_handle(), SOL_SOCKET,  SO_KEEPALIVE, &keepalive, sizeof(keepalive));
			setsockopt(socket_->native_handle(), IPPROTO_TCP, TCP_KEEPALIVE, &timeout_seconds, sizeof(timeout_seconds));
#else
			  // assume everything else is posix
			//  struct timeval tv;
			 // tv.tv_sec  = timeout_seconds;
			 // tv.tv_usec = 0;
			int keepalive= 1;	
			int keepidle = 300; // 如该连接在300秒(5minutes)内没有任何数据往来,则进行探测  
			int keepinterval = 3; // 探测时发包的时间间隔为5 秒  
			int keepcount = 3; // 探测尝试的次数。如果第1次探测包就收到响应了,则后2次的不再发。  
		  
	 		 setsockopt(socket_->native_handle(), SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));  
			 setsockopt(socket_->native_handle(), SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));  
			 setsockopt(socket_->native_handle(), SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));  
			 setsockopt(socket_->native_handle(), SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));  		  
		  
		  
//			  setsockopt(socket_->native_handle(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
//			  setsockopt(socket_->native_handle(), SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
		  


//	unique_id_ = container_.unique_id();
	//auto remote_endpoint = new_session->socket.remote_endpoint();
	auto remote_endpoint = socket_->remote_endpoint();
	remote_address_ = remote_endpoint.address().to_string();
	auto itClient = sm_client_failtimes_.find( remote_address_ ) ;
	if( itClient == sm_client_failtimes_.end() ){
		sm_client_failtimes_.insert( std::pair<std::string, int>( remote_address_, 0) );
	}else{
		if( itClient->second > 400 ){
			std::string t("Reject Client:"); t += remote_address_; t +=" for too many connection requests (over 400)!";
			logger_( MessengerBase::sm_total_clients_, t);
			socket_->close();
			return;
		}
	}

	remote_address_port_ = remote_address_;	remote_address_port_ +=":"; 
	remote_address_port_ += boost::lexical_cast<std::string>( remote_endpoint.port() ); 
	++MessengerBase::sm_total_clients_;

	std::string t("New Client:"); t += remote_address_port_; t +="  has connected!";
	logger_( MessengerBase::sm_total_clients_, t);

	container_.push( std::enable_shared_from_this<MessengerBase>::shared_from_this() );
}

/*Message Server*/
ServerBase::ServerBase( boost::asio::ip::tcp::endpoint &endpoint, const std::function< void(int, const std::string &) > & logger)
	: io_service_(), acceptor_(io_service_),logger_(logger),container_(SHANNON_MESSAGE_CONNECTIONS_ALLOWED), running_threads_(0),stopped_(false)
{ 
	__init(endpoint);
}

ServerBase::ServerBase( unsigned short port, const std::function< void(int, const std::string &) > & logger)
	: io_service_(), acceptor_(io_service_),logger_(logger),
	container_(SHANNON_MESSAGE_CONNECTIONS_ALLOWED),running_threads_(0),stopped_(false)
{
	boost::asio::ip::tcp::endpoint endpoint( boost::asio::ip::tcp::v4(), port );
	__init( endpoint );
}

void ServerBase::__init( boost::asio::ip::tcp::endpoint &endpoint ){
	boost::system::error_code ec;
//	 acceptor_.set_option( boost::asio::ip::tcp::acceptor::reuse_address(true) );
	acceptor_.open(boost::asio::ip::tcp::v4(), ec) || acceptor_.bind( endpoint, ec) || acceptor_.listen(boost::asio::socket_base::max_connections, ec);
	if( ec ){
		std::string t("Could not listen the tcp port:"); t += boost::lexical_cast<std::string>( endpoint.port() ); t +="  with error: "; t += ec.message();
		logger_(-1, t);
	}	
}
void ServerBase::_enter_run_loop(bool flag){
	running_threads_.fetch_add(1);
	do{
		if( flag ) _Listen();
		
		try{
			io_service_.run();
		}catch(...){
			logger_( container_.size(), "Unexpected exception caught in ServerBase ListenLoop!");
			std::cerr <<  "===> Unexpected exception caught in " << BOOST_CURRENT_FUNCTION << std::endl 
				<< boost::current_exception_diagnostic_information()<< std::endl; 
		}

		while( !io_service_.stopped() ){
			UnifiedSleep(200);
			if( flag ) io_service_.stop();
		}
		if (stopped_.load(boost::memory_order_relaxed)) break;

		if( flag ) io_service_.reset();
	}while(flag);
	running_threads_.fetch_sub(1);
}
void ServerBase::run( bool detached /* = false */) {
	if( detached ){
		std::thread t( std::bind(&ServerBase::_enter_run_loop, this, true ) );
		t.detach();
	}else{
		_enter_run_loop(true);
	}
}

void ServerBase::heart_beating_loop(int sec){
	std::thread t(std::bind(&ServerBase::_heart_beating_loop, this, sec));
	t.detach();
}
void ServerBase::_heart_beating_loop(int sec){
	while (stopped_.load(boost::memory_order_relaxed)){
		heart_beating();
		UnifiedSleep(sec * 1000);
	}
}
void ServerBase::heart_beating(){
	MessageInfo<char> msg;
	std::size_t i=0, n = container_.size();
	while( i < n ){
		std::shared_ptr<MessengerBase> client = container_.at(i++);
		if( client )
			client->Post<char>( msg, SHANNON_MESSAGE_HEART_BEATING_TYPE, true);
	}
}
void ServerBase::stop(){
	
	stopped_.store(true);

	boost::system::error_code ec;
	acceptor_.close(ec);
	io_service_.stop();
	
	MessageInfo<char> msg;
	std::size_t i=0;
	while( i < container_.size() ){
		std::shared_ptr<MessengerBase> client = container_.at(i++);
		if( client )
			client->Post<char>( msg, SHANNON_MESSAGE_ACTIVE_CLOSE_TYPE, true);
	}
}




}//namespace Message
}//namespace Shannon



