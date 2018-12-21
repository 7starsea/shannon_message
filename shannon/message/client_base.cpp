///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#include "client_base.h"
#include <boost/bind.hpp>
#include <boost/predef.h>

#if BOOST_OS_WINDOWS
#include <Mstcpip.h>
#include <Winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#endif
namespace Shannon{
	namespace Message{



ClientBase::ClientHelper::ClientHelper(std::size_t max_threads)
	:Container< std::shared_ptr<ClientBase> >(0),
	max_threads_(max_threads),running_threads_(0),stopped_(false),
	work_io_service_(),
	worker_(new boost::asio::io_service::work(work_io_service_))
{}

ClientBase::ClientHelper::~ClientHelper(){
	worker_.reset();
	if( work_thread_ && work_thread_->joinable() ){
		work_thread_->join();
	}
}


void ClientBase::ClientHelper::stop(){
	clear();
	worker_.reset();

	stopped_.store(true);
	if( !work_io_service_.stopped() ) work_io_service_.stop();

	while(running_threads_.load() > 0){
		UnifiedSleep(100);
	}
	UnifiedSleep(100);
}
void ClientBase::ClientHelper::_run_loop(bool flag){
	
	running_threads_.fetch_add(1);
#ifdef SHANNON_MESSAGE_CLIENT_DEBUG
	std::cout<<"===> client base helper start!"<<std::endl;
#endif

	do{
		while( !work_io_service_.stopped() ){
			UnifiedSleep(200);
			if( flag ) work_io_service_.stop();
		}
		if( flag ) work_io_service_.reset();

		try{
			work_io_service_.run();
		}catch(...){
			std::cerr <<  "===> Unexpected exception caught in " << BOOST_CURRENT_FUNCTION << std::endl 
				<< boost::current_exception_diagnostic_information()<< std::endl; 
		}
		
		if( stopped_.load() ) break;
	}while(flag);

#ifdef SHANNON_MESSAGE_CLIENT_DEBUG
	std::cout<<"===> client base helper stop!"<<std::endl;
#endif
	running_threads_.fetch_sub(1);
}
void ClientBase::ClientHelper::push(const std::shared_ptr<ClientBase>& session ){
	Container< std::shared_ptr<ClientBase> >::push( session );

	std::size_t running_threads = running_threads_.load();
	bool new_thread = running_threads < max_threads_ && Container< std::shared_ptr<ClientBase> >::size() >= 3 * running_threads;

	if( new_thread ){
		std::thread tr(std::bind(&ClientHelper::_run_loop, this, false )  );
		tr.detach();
	}
}
void ClientBase::ClientHelper::run_once(){
	if( !work_thread_ ){
		work_thread_ = std::shared_ptr<std::thread>( new std::thread(std::bind(&ClientHelper::_run_loop, this, true  )  ) );
	}
}



ClientBase::ClientHelper ClientBase::sm_client_helper(2);
void ClientBase::safe_exit(){
	ClientBase::sm_client_helper.stop();
}
ClientBase::ClientBase(const std::string & listen_addr,  
						const std::function< void(int, const std::string &) > & logger )
						:std::enable_shared_from_this<ClientBase>(),
						listen_addr_(listen_addr), 
                        socket_(new boost::asio::ip::tcp::socket( ClientBase::sm_client_helper.get_io_service()) ),
						timeout_deadline_(  ClientBase::sm_client_helper.get_io_service() ),
						try_connect_deadline_(  ClientBase::sm_client_helper.get_io_service() ),
						endpoint_(),
                        data_received_(0),connect_timeout_(0),buffer_type_(SHANNON_MESSAGE_NULL_VOID_TYPE), 
						is_connected_(false), auto_reconnected_(true), 
						logger_(logger)
{
	const std::size_t found = listen_addr_.find_last_of(":");
	if( found !=std::string::npos ){
		std::string ip = listen_addr_.substr(0, found );
		std::string port_str = listen_addr_.substr( found+1 );
		unsigned short port = std::atoi( port_str.c_str() );
		try{
			endpoint_ = boost::asio::ip::tcp::endpoint( boost::asio::ip::address::from_string( ip ), port);
		}catch(std::exception&){
			endpoint_ = boost::asio::ip::tcp::endpoint();
			logger_(0, "Invalid Listen Address:" + listen_addr_ );		
		}
	}
	else{
		logger_(0, "Invalid Listen Address:" + listen_addr_ );
	}
}

ClientBase::~ClientBase(){
    boost::system::error_code ec;
	timeout_deadline_.cancel(ec);
	try_connect_deadline_.cancel(ec);
    socket_->close(ec);
}
void ClientBase::stop(){
	auto_reconnected_.store(false);

	boost::system::error_code ec;
	socket_->shutdown( boost::asio::ip::tcp::socket::shutdown_both, ec);

    socket_->close(ec);
    ClientBase::sm_client_helper.pop( shared_from_this() );
}

void ClientBase::Listen(){
	ClientBase::sm_client_helper.run_once();
	if(!is_connected_.load(boost::memory_order_consume)){
		ClientBase::sm_client_helper.push( shared_from_this() );
		__Connect();
	}
}



void ClientBase::___Connect(const boost::system::error_code& error  ){
	if( error == boost::asio::error::operation_aborted ){
		return;
	}

	if( !auto_reconnected_.load(boost::memory_order_consume) ){
		ClientBase::sm_client_helper.pop( shared_from_this() );
		return;
	}

	 // Set a deadline for the connect operation.
	if( ! ( socket_->is_open() && socket_->non_blocking() ) ){
		boost::system::error_code ec;
		if( socket_->is_open() ) socket_->close( ec ) ;

		timeout_deadline_.expires_from_now(boost::posix_time::seconds(8));
		timeout_deadline_.async_wait(boost::bind(&ClientBase::__CheckDeadline, shared_from_this(), boost::asio::placeholders::error ));

		socket_->async_connect( endpoint_,
          boost::bind(&ClientBase::__OnConnect, shared_from_this(), boost::asio::placeholders::error ));

	}
}

void ClientBase::__OnConnect(const boost::system::error_code& error){
	boost::system::error_code ec;
    //socket_->connect( endpoint_, ec ) || 
//	if( error != boost::asio::error::operation_aborted )
		timeout_deadline_.cancel(ec);
	if( !socket_->is_open() ){
	
	}else if( error ){
#ifdef SHANNON_MESSAGE_CLIENT_DEBUG
	std::cout<<"===> OnConnect Error:"<<error.message()<<std::endl;
#endif
		
		socket_->close(ec);
	}else{
		socket_->non_blocking( true, ec );
		if( ec ){
			socket_->close(ec);
		}else{
			
			
			// platform-specific switch
#if BOOST_OS_WINDOWS
			  // use windows-specific time
			
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
		  
			
			logger_( data_received_ + 1 , "Connected to remote server " + listen_addr_ + ".");
			
			this->_Listen();
			is_connected_.store(true);
		}
	}

	if(!socket_->is_open()){
		try_connect_deadline_.expires_from_now(boost::posix_time::seconds(2));
		try_connect_deadline_.async_wait(boost::bind(&ClientBase::___Connect, shared_from_this(), boost::asio::placeholders::error ));
	}

}
void ClientBase::__CheckDeadline( const boost::system::error_code& error )
 {
	 if( error ){
#ifdef SHANNON_MESSAGE_CLIENT_DEBUG
	std::cout<<"===> CheckDeadline Error:"<<error.message()<<std::endl;
#endif
		return;
	 }  
    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
   if (timeout_deadline_.expires_at() > boost::asio::deadline_timer::traits_type::now()){
		std::cout<<"===> Unexpected error in ClientBase CheckDeadline!"<<std::endl;
    }

#ifdef SHANNON_MESSAGE_CLIENT_DEBUG
	std::cout<<"===> Connect Deadline Passed!!"<<std::endl;
#endif
	++connect_timeout_;

	if(connect_timeout_ >=80){
		auto_reconnected_.store(false);
		logger_(-data_received_-1, "The destination:"+listen_addr_+" is unreachable!");
	}

	if( ! is_connected_.load(boost::memory_order_consume) ){
      // The deadline has passed. The socket is closed so that any outstanding
      // asynchronous operations are canceled.
		boost::system::error_code ec;
	     socket_->close(ec);
	}
 }


void ClientBase::__Connect(  ){
	if( ! endpoint_.port() ) return;
	is_connected_.store(false);
    
	logger_( -data_received_ - 1, "trying to connect to remote server " + listen_addr_ + " ... ");
	
	boost::system::error_code ec;
    if( socket_->is_open() )	socket_->close(ec);
	ec.clear();
	connect_timeout_ = 0;
	___Connect(ec);
}

//监听，定义为虚函数，需要子类继承
void ClientBase::_Listen(){
	/////ListenType
	boost::asio::async_read(*(socket_.get()),
            boost::asio::buffer( (void*)(&buffer_type_), sizeof(buffer_type_ ) ),
            boost::bind(&ClientBase::_OnListenType,
            shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

//实现异步的对监听到的消息的处理
void ClientBase::_OnListenType(const boost::system::error_code& error, std::size_t bytes_transferred ){
	G_UNUSED(bytes_transferred);
	if( error ){
		logger_(-data_received_-1, " ClientBase::OnListenType Failed. Disconnected from the server!" );
		this->_ReConnect();
		return;
	}
	if( SHANNON_MESSAGE_ACTIVE_CLOSE_TYPE == buffer_type_ ){
		logger_(-data_received_-1, std::string("Received SHANNON_MESSAGE_ACTIVE_CLOSE_TYPE!"));
		this->_ReConnect();///active close
	}
}
	
void ClientBase::_ReConnect(  ){
    boost::system::error_code ec;
    is_connected_.store(false);
	socket_->shutdown( boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket_->close(ec);
    
	if( !auto_reconnected_.load(boost::memory_order_consume) ) return;

//    socket_ = std::shared_ptr<boost::asio::ip::tcp::socket>( new boost::asio::ip::tcp::socket( ClientBase::sm_client_helper.get_io_service()) );
    this->__Connect();
}









}//namespace Message
}//namespace Shannon

