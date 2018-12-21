///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef SHANNON_MESSAGE_HELPER_CONSOLE_CLIENT_INITIALIZER_H
#define SHANNON_MESSAGE_HELPER_CONSOLE_CLIENT_INITIALIZER_H
#include <iostream>
#include <vector>
#include <map>
#include <mutex>
///#include <boost/thread/shared_mutex.hpp>
#include <boost/thread.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/find.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/mpl/replace_if.hpp>
#include <boost/mpl/empty.hpp>

#include "console_struct.h"
#include "subscribe_client.hpp"

namespace Shannon{
	namespace Message{

		template<typename VecDataStruct>
		class ConsoleClientInitializer{
		private:

			template<typename T>
			struct Galois_WApply{
				template<typename GT>
				struct apply{
					typedef boost::is_same<typename GT::type, T> type;
				};
			};

			struct RemoveValueHelper{
				template<typename T>
				struct apply{
					typedef typename T::type type;
				};
			};

			struct AddConsoleWrapperHelper{
				
				template<typename T>
				struct apply{
					typedef ConsoleDataStruct<typename T::type> type;
				};

			};

				struct _InstanceBase{
				const std::vector<std::string> listen_keys;
				const Shannon::Message::MessageTypeGroup  message_group;
				Shannon::Message::MessageResumeType resume_type;
				const int index;
				int data_received;
				bool is_init;
				explicit _InstanceBase(const std::vector<std::string> & t_keys,
							  const Shannon::Message::MessageTypeGroup & t_group,
							  Shannon::Message::MessageResumeType t_resume,
							  const int t_index
							  )
							  :listen_keys(t_keys), message_group(t_group),resume_type(t_resume), index(t_index), data_received(0),is_init(false){}
	
			};

			template<typename DataStruct>
			struct _Instance : public _InstanceBase{
				typedef DataStruct type;
				std::map<std::string, DataStruct> data_storage;
				std::function< void(DataStruct*)> updater;
				explicit _Instance(const std::vector<std::string> & t_keys,
							  const Shannon::Message::MessageTypeGroup & t_group,
							  Shannon::Message::MessageResumeType t_resume,
							  const int t_index		  )
							  :_InstanceBase(t_keys, t_group, t_resume,  t_index){}
			};
		public:
			ConsoleClientInitializer(const std::string & console_address, const std::function< void(int, const std::string &) > logger = &Shannon::Message::UnifiedLoggerForClient);
			~ConsoleClientInitializer();

			inline void SetInitTrue(){ is_init_ = true; }
			bool IsInit();

			template<typename DataStruct>
			std::map<std::string, DataStruct> RtnInitData();

			template<typename DataStruct>
			void Register(const std::vector<std::string> & t_keys, MessageResumeType t_resume = SHANNON_MESSAGE_RESUME_RESTART);

			template<typename DataStruct>
			void RegisterUpdater(const std::function< void(DataStruct*)> & t_updater);

			template<typename DataStruct>
			void OnReceiveData( ConsoleDataStruct<DataStruct>*);

		private:
			typedef typename boost::mpl::transform<VecDataStruct, RemoveValueHelper >::type void_removed_VecDataStruct;
			BOOST_MPL_ASSERT_NOT((boost::mpl::empty<void_removed_VecDataStruct>));

			typedef typename boost::mpl::transform<VecDataStruct, AddConsoleWrapperHelper >::type adjust_client_vec_datastruct;

			typedef typename Shannon::Message::SubscribeClient< adjust_client_vec_datastruct, Shannon::Message::ConsoleSubscribeField >::type ConsoleClientInitializerClient;

			const std::string console_address_;
			std::string client_name_;
			std::shared_ptr< ConsoleClientInitializerClient > initializer_client_;

			bool is_init_;

			std::map<Shannon::Message::MessageType, _InstanceBase* > instances_;
			std::vector< int > instance_type_indexes_;
			std::mutex m_mutex;
		public:
			inline void start(){
				initializer_client_->Listen();
			}
			inline void stop(){
				initializer_client_->stop();
			}
			inline void SendFeedbackMsg(const Shannon::Message::ConsoleFeedbackField & feedback){
				initializer_client_->SendMessage(&feedback, SHANNON_MESSAGE_CONSOLE_FEEDBACK_MSG_TYPE);
			}
			void SetClientName(const std::string & client_name){
				Shannon::Message::ConsoleSubscribeField console_field;
				strncpy(console_field.client_name, client_name.c_str(),sizeof(console_field.client_name)-1);
				client_name_ = client_name;
				initializer_client_->SetTplContent(console_field);
			}
		};

		template<typename VecDataStruct>
		ConsoleClientInitializer<VecDataStruct>::ConsoleClientInitializer(const std::string & console_address , const std::function< void(int, const std::string &) > logger)
			:console_address_(console_address),	initializer_client_( new ConsoleClientInitializerClient(console_address_, logger)),
			is_init_(false){}

		template<typename VecDataStruct>
		ConsoleClientInitializer<VecDataStruct>::~ConsoleClientInitializer(){
			for(auto it = instances_.begin(); it != instances_.end(); ++it ){
				delete it->second;
				it->second = NULL;
			}
		}

		template<typename VecDataStruct>
		bool ConsoleClientInitializer<VecDataStruct>::IsInit(){
			std::lock_guard<std::mutex> write_lock(m_mutex);
			is_init_ = true;
			for(auto it = instances_.begin(); it != instances_.end(); ++it){
				if( ! it->second->is_init ){ is_init_ = false; break; }
			}
			return is_init_;
		}

		template<typename VecDataStruct>
		template<typename DataStruct>
		void ConsoleClientInitializer<VecDataStruct>::Register(const std::vector<std::string> & t_keys, MessageResumeType t_resume)
		{
			typedef typename boost::mpl::find_if<VecDataStruct, Galois_WApply<DataStruct> >::type found_type;
			typedef typename boost::mpl::deref<found_type>::type galois_w_type;
			typedef boost::is_same<typename galois_w_type::type, DataStruct > value_type;
			BOOST_STATIC_ASSERT_MSG( value_type::value, "Invalid/Incompatible DataStruct" );
			BOOST_STATIC_ASSERT_MSG( !boost::is_same<void, DataStruct>::value, "The DataStruct could not be void type" );

			Shannon::Message::MessageTypeGroup  t_group;
			t_group.subscribe = 200 + galois_w_type::value * 10 + 1;
			t_group.unsubscribe = 200 + galois_w_type::value * 10 + 2;
			t_group.update = 200 + galois_w_type::value * 10 + 3;

			initializer_client_->template Register< ConsoleDataStruct<DataStruct> >( t_keys, t_group, t_resume, std::bind(&ConsoleClientInitializer<VecDataStruct>::OnReceiveData<DataStruct>, this, std::placeholders::_1 ) );

			auto it = std::find(instance_type_indexes_.begin(), instance_type_indexes_.end(), found_type::pos::value);
			if( it == instance_type_indexes_.end() && instances_.find( t_group.update ) == instances_.end() ){
				_Instance<DataStruct> * t = new _Instance<DataStruct>(t_keys, t_group, t_resume, found_type::pos::value);
				instances_.insert( std::pair<Shannon::Message::MessageType, _InstanceBase*>(  t_group.update , t ) );
				instance_type_indexes_.push_back( found_type::pos::value );
			}else{
				std::cout<<"===> You have already registered the datastruct: "<<typeid(DataStruct).name()<<" or the message update type: "<<t_group.update<<std::endl;
			}
		}

		template<typename VecDataStruct>
		template<typename DataStruct>
		void ConsoleClientInitializer<VecDataStruct>::RegisterUpdater(const std::function< void(DataStruct*)> & t_updater){

			typedef typename boost::mpl::find_if<VecDataStruct, Galois_WApply<DataStruct> >::type found_type;
			typedef typename boost::mpl::deref<found_type>::type galois_w_type;
			typedef boost::is_same<typename galois_w_type::type, DataStruct> value_type;
			BOOST_STATIC_ASSERT_MSG( value_type::value, "Invalid/Incompatible DataStruct" );
			BOOST_STATIC_ASSERT_MSG( !boost::is_same<void, DataStruct>::value, "The DataStruct could not be void type" );


			std::lock_guard<std::mutex> write_lock(m_mutex);
			auto it = instances_.begin(); 
			for(;it != instances_.end(); ++it){
				if( found_type::pos::value == it->second->index )
					break;
			}
			if( it != instances_.end() && t_updater ){
				_Instance<DataStruct> * t = static_cast< _Instance<DataStruct>* >( it->second );
				t->updater =  t_updater ;

				////if there is data in data_storage, we need to update them
				for(auto itData = t->data_storage.begin(); itData != t->data_storage.end(); ++itData ){
					t->updater( &(itData->second) );
				}
				t->data_storage.clear();
			}
		}


		template<typename VecDataStruct>
		template<typename DataStruct>
		void ConsoleClientInitializer<VecDataStruct>::OnReceiveData(ConsoleDataStruct<DataStruct>* data){
			typedef typename boost::mpl::find_if<VecDataStruct, Galois_WApply<DataStruct> >::type found_type;
			typedef typename boost::mpl::deref<found_type>::type galois_w_type;
			typedef boost::is_same<typename galois_w_type::type, DataStruct> value_type;
			BOOST_STATIC_ASSERT_MSG( value_type::value, "Invalid/Incompatible DataStruct" );
			BOOST_STATIC_ASSERT_MSG( !boost::is_same<void, DataStruct>::value, "The DataStruct could not be void type" );

			std::lock_guard<std::mutex> write_lock(m_mutex);
			auto it = instances_.begin();
			for(; it != instances_.end(); ++it){
				if( found_type::pos::value == it->second->index )
					break;
			}
			if( it != instances_.end() ){
				_Instance<DataStruct> * t = static_cast< _Instance<DataStruct>* >( it->second );

				if( is_init_ && t->updater){
					t->updater( &(data->data) );
				}else{
					auto itData = t->data_storage.find( data->data.key_id() );
					if( itData == t->data_storage.end() ){
						t->data_storage.insert(  std::pair<std::string, DataStruct>(data->key_id(), data->data )  );
					}else{
						itData->second = data->data;
					}
				}
				t->data_received += 1;
				if( data->is_last ){
					t->is_init = true;

					///// send feedback message
					Shannon::Message::ConsoleFeedbackField feedback;
					strncpy(feedback.id, client_name_.c_str(), sizeof( feedback.id ) - 1 );
					feedback.is_error = false;

					std::string logger_name = (typeid(DataStruct).name() );
					std::string msg("(");	msg.reserve(logger_name.size() + 22);

					std::size_t pos = logger_name.find(' ');
					if( pos == std::string::npos ) pos = 0;
					while( pos < logger_name.size() ){
						char c = logger_name[pos];
						if( ('a' <= c && c<= 'z') || ('A'<=c && c <='Z') ){
							msg.append(1, c);
						}
						++pos;
					}

					msg +=") Data Received: "; 
					msg += boost::lexical_cast<std::string>( t->data_received  );
					strncpy(feedback.message, msg.c_str(), sizeof(feedback.message)-1);
					initializer_client_->SendMessage(&feedback, SHANNON_MESSAGE_CONSOLE_FEEDBACK_MSG_TYPE);

					t->data_received = 0;
				}	
			}
		}

		template<typename VecDataStruct>
		template<typename DataStruct>
		std::map<std::string, DataStruct> ConsoleClientInitializer<VecDataStruct>::RtnInitData(){

			typedef typename boost::mpl::find_if<VecDataStruct, Galois_WApply<DataStruct> >::type found_type;
			typedef typename boost::mpl::deref<found_type>::type galois_w_type;
			typedef boost::is_same<typename galois_w_type::type, DataStruct> value_type;
			BOOST_STATIC_ASSERT_MSG( value_type::value, "Invalid/Incompatible DataStruct" );
			BOOST_STATIC_ASSERT_MSG( !boost::is_same<void, DataStruct>::value, "The DataStruct could not be void type" );

			std::lock_guard<std::mutex> write_lock(m_mutex);
			auto it = instances_.begin();
			for(; it != instances_.end(); ++it){
				if( found_type::pos::value == it->second->index )
					break;
			}
			std::map<std::string, DataStruct> tmpData;
			if( it != instances_.end() ){
				_Instance<DataStruct> * t = static_cast< _Instance<DataStruct>* >( it->second );
				tmpData = t->data_storage;
		
				//We want to clear the data_storage for storing new updated data
				t->data_storage.clear();
			}
			return tmpData;
		}


}//namespace Message
}//namespace Shannon
#endif
