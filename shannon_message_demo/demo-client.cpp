///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#include <iostream>
#include <vector>
#include <boost/mpl/vector.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


#include "demo-message-datastruct.h"

/// We need BOOST_REGEX_NO_LIB here, see https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/using.html
#define BOOST_REGEX_NO_LIB
#include <shannon/message/client.hpp>
#include <shannon/message/helper/subscribe_struct.h>


typedef boost::mpl::vector<TestSTructFirst, TestSTructSecond> VecTestStructs;

typedef Shannon::Message::Client< VecTestStructs >::type MyBaseClient;
///typical usage:
/// Step 0. 构造 MyBaseClient(listen_address, logger)
/// Step 1. 注册回调函数 Register<DataStruct>(const MessageTypeGroup & t_group,const std::function< void(DataStruct*)> & t_updater)
/// Step 2. 调用Listen() [之后client会尝试连接server, 如果连接成功，则会调用虚函数 _Listen(); MyBaseClient::_Listen() 功能是启动监听Server消息]
///											如果想在监听Server消息前，先给Server发送些消息，则需要 写一个类重载 虚函数 _Listen() 如下面的示例

///extending MyBaseClient by providing your own _Listen method
class MySubscribeClient : public MyBaseClient{
private:
	struct _SubCache{
		std::vector<std::string> listen_keys;
		Shannon::Message::MessageType  subscribe;
		Shannon::Message::MessageResumeType resume_type;
		_SubCache(const std::vector<std::string> & t_keys,
			const Shannon::Message::MessageTypeGroup & t_group,
			Shannon::Message::MessageResumeType t_resume)
			:listen_keys(t_keys), subscribe(t_group.subscribe),resume_type(t_resume){}
	};
public:
	MySubscribeClient(const std::string & listen_addr,  
		const std::function< void(int, const std::string &) > logger = &Shannon::Message::UnifiedLoggerForClient
		):MyBaseClient(listen_addr,   logger) {}

	template<typename DataStruct>
	void Register(const std::vector<std::string>& t_keys,
		const Shannon::Message::MessageTypeGroup & t_group,
		Shannon::Message::MessageResumeType t_resume,
		const std::function< void(DataStruct*)>& t_updater){

			///Call me at the beginning usually
			MyBaseClient::Register<DataStruct>(t_group, t_updater);

			subcribe_cache_.push_back( _SubCache(t_keys, t_group, t_resume) );
	}
protected:
	///do some pre send message before listen from server
	virtual void _Listen(){
		Shannon::Message::MessageInfo<Shannon::Message::SubscribeField> subscribe_struct;
		memset(&(subscribe_struct.content), 0, sizeof(subscribe_struct.content));

		for(auto it=subcribe_cache_.begin(); it != subcribe_cache_.end(); ++it ){

			subscribe_struct.content.resume_type = it->resume_type;

			int succ=0;
			for(auto itKey = it->listen_keys.begin(); itKey != it->listen_keys.end(); ++itKey ){
				strncpy(subscribe_struct.content.id, itKey->c_str(), sizeof(subscribe_struct.content.id) - 1);
				if( this->SendMessage<Shannon::Message::SubscribeField>( &subscribe_struct, it->subscribe ) ) ++succ;
				else break;	///send message failed
			}
			std::string strmsg = "Sent: "; 
			strmsg += boost::lexical_cast<std::string>( succ ) ; strmsg += " Succeed; ";
			strmsg += boost::lexical_cast<std::string>( it->listen_keys.size() ) ; strmsg += " Total; ";

			MyBaseClient::logger_(1, strmsg );
			it->resume_type = SHANNON_MESSAGE_RESUME_QUICK;  ////之后重新订阅的话使用Quick Mode	
		}

		////Send SHANNON_MESSAGE_NO_MORE_TYPE; //indicate the server that all subscribe key has been sent;
		////must be called if you use restart mode
		this->SendMessage(SHANNON_MESSAGE_NO_MORE_TYPE);

		///Call me at the end usually; start listening from server
		MyBaseClient::_Listen();
	}
private:
	std::vector<_SubCache> subcribe_cache_;
};

////By default, Shannon::Message library provide a client doing the same stuff as MySubscribeClient
///you need to include
#include <shannon/message/helper/subscribe_client.hpp>
/// Use Shannon::Message::SubscribeClient< VecTestStructs >::type MySubscribeClient2;


///实现一个基本的DemoParams的更新函数，只在标准输出流中打印收到的参数

void DemoUpdater1(TestSTructFirst * demo_params){
	std::cout<<"TestSTructFirst: "<< "Time: " << demo_params->update_time << ", ID:" << demo_params->id << std::endl;
}

void DemoUpdater2(TestSTructSecond * demo_params)
{
	std::cout<<"TestSTructSecond: "<< "Time: " << demo_params->update_time << ", ID:" << demo_params->id << std::endl;
}



int main()
{
	std::vector<std::string> instrument_list;

	instrument_list.push_back("510050");
	instrument_list.push_back("10000585");
	instrument_list.push_back("10000586");
	
	///定义与message相关的消息类型组

	Shannon::Message::MessageTypeGroup msg_group;
	msg_group.subscribe	=  101;
	msg_group.unsubscribe = 102;
	msg_group.update		= 103;
	//创建MessageClient类实例
	
	std::shared_ptr< MySubscribeClient >message_client(
		new MySubscribeClient("127.0.0.1:4444"));


	msg_group.subscribe	= 201;
	msg_group.unsubscribe = 202;
	msg_group.update		= 203;
	message_client->Register<TestSTructFirst>( instrument_list, msg_group, SHANNON_MESSAGE_RESUME_RESTART, DemoUpdater1);

	msg_group.subscribe	= 211;
	msg_group.unsubscribe = 212;
	msg_group.update		= 213;
	message_client->Register<TestSTructSecond>( instrument_list, msg_group, SHANNON_MESSAGE_RESUME_RESTART, DemoUpdater2);


	///监听消息

	message_client->Listen();

	while(true){ 
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000 * 100)); 
	}

	return 0;
}