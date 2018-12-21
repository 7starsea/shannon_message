///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef SHANNON_MESSAGE_CLIENT_HELPER_SUBSCRIBE_HPP
#define SHANNON_MESSAGE_CLIENT_HELPER_SUBSCRIBE_HPP

#include <shannon/message/client.hpp>
#include "subscribe_struct.h"
namespace Shannon{
	namespace Message{

		////Requirement for SendStruct: with property: id and resume_type
	template<typename T, typename SendStruct=SubscribeField>
	struct SubscribeClient {
		typedef typename Client<T>::type _MyBaseClient;

		class _MySubscribeClient : public _MyBaseClient{
		private:
				struct _SubCache{
					std::vector<std::string> listen_keys;
					MessageType  subscribe;
					MessageResumeType resume_type;
					_SubCache(const std::vector<std::string> & t_keys,
							const MessageTypeGroup & t_group,
							MessageResumeType t_resume)
							:listen_keys(t_keys), subscribe(t_group.subscribe),resume_type(t_resume){}
				};
		public:
			_MySubscribeClient(const std::string & listen_addr,  
				 const std::function< void(int, const std::string &) > logger = &UnifiedLoggerForClient
				 ):_MyBaseClient(listen_addr,   logger), tpl_send_content_(){
					 memset(&tpl_send_content_, 0, sizeof(SendStruct));
			}

			inline void SetTplContent(const SendStruct & tpl_content){
				memcpy(&tpl_send_content_, &tpl_content, sizeof(SendStruct));
			}

			template<typename DataStruct>
			void Register(const std::vector<std::string>& t_keys,
				  const MessageTypeGroup & t_group,
				  MessageResumeType t_resume,
				  const std::function< void(DataStruct*)>& t_updater){

					///Call me at the beginning usually
				bool already_registered = false;
				
				for (auto itCache = subcribe_cache_.begin(); itCache != subcribe_cache_.end(); ++itCache)
				{
					if ( (*itCache).subscribe == t_group.subscribe )
					{
						if(t_resume == SHANNON_MESSAGE_RESUME_RESTART){
							(*itCache).resume_type = SHANNON_MESSAGE_RESUME_RESTART;
						}
						(*itCache).listen_keys.insert((*itCache).listen_keys.end(), t_keys.begin(), t_keys.end() );
						already_registered = true;
						break;
					}
				}
				if(!already_registered){
					_MyBaseClient::template Register<DataStruct>(t_group, t_updater);
					subcribe_cache_.push_back( _SubCache(t_keys, t_group, t_resume) );
				}
			}
		protected:
				///do some pre send message before listen from server
			virtual void _Listen(){
				MessageInfo<SendStruct> msg_info;
				memcpy(&(msg_info.content), &tpl_send_content_, sizeof(msg_info.content));

				for(auto it=subcribe_cache_.begin(); it != subcribe_cache_.end(); ++it ){

					msg_info.content.resume_type = it->resume_type;

					int succ=0;
					for(auto itKey = it->listen_keys.begin(); itKey != it->listen_keys.end(); ++itKey ){
						strncpy(msg_info.content.id, itKey->c_str(), sizeof(msg_info.content.id) - 1);
						if( this->template SendMessage<SendStruct>( &msg_info, it->subscribe ) ) ++succ;
						else break;
					}
					std::string strmsg = "Sent: "; 
					strmsg += boost::lexical_cast<std::string>( succ ) ; strmsg += " Succeed; ";
					strmsg += boost::lexical_cast<std::string>( it->listen_keys.size() ) ; strmsg += " Total; ";

					_MyBaseClient::logger_(1, strmsg );
					it->resume_type = SHANNON_MESSAGE_RESUME_QUICK;  ////之后重新订阅的话使用Quick Mode	
				}
				
				////Send SHANNON_MESSAGE_NO_MORE_TYPE
				this->SendMessage(SHANNON_MESSAGE_NO_MORE_TYPE);

					///Call me at the end usually
				_MyBaseClient::_Listen();
			}
		private:
			SendStruct tpl_send_content_;
			std::vector<_SubCache> subcribe_cache_;
		};



		typedef _MySubscribeClient type;
	};



}//namespace Message
}//namespace Shannon



#endif