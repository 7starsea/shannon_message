///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef SHANNON_MESSAGE_CONTAINER_H
#define SHANNON_MESSAGE_CONTAINER_H

#include <mutex>
#include <set>
#include <functional>
#include <boost/atomic.hpp>
#include <boost/mpl/max_element.hpp>
#include <boost/mpl/transform_view.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/sizeof.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/lambda.hpp>

#ifndef G_UNUSED
#define G_UNUSED(x) (void)x
#endif

namespace Shannon{
	namespace Message{

		class SpinLockMutex {
		public:
			SpinLockMutex(){}
			inline bool try_lock(){
				return false == locked_.test_and_set(boost::memory_order_acquire);
			}
			inline void lock() {
				while (locked_.test_and_set(boost::memory_order_acquire));
			}
			inline void unlock() {
				locked_.clear(boost::memory_order_release);
			}
		protected:
			boost::atomic_flag locked_;
		};


		template<typename VecDataStruct>
		struct MaxSizeTypeHelper{
			typedef typename boost::mpl::max_element<
			  boost::mpl::transform_view< VecDataStruct, boost::mpl::sizeof_<boost::mpl::_1> >
			>::type iter;

			typedef typename boost::mpl::deref<typename iter::base>::type type;
			const static int value =  boost::mpl::deref<iter>::type::value;
		};


		template<typename VecDataStruct>
		struct AddTypeWrapperHelper{
			template<typename T>
			struct apply{
				struct _type_wrapper_{
					typedef T type;
					typedef typename boost::mpl::find<VecDataStruct, T>::type found_type;
					typedef  boost::mpl::int_<found_type::pos::value> index;
				};
				typedef _type_wrapper_ type;
			};
		};

		
		template<typename MessageInstance>
		class Container{
		public:
			Container(std::size_t max)
				:max_num_(max),unique_id_(1){}
			void push( const MessageInstance &  session );
			void pop( const MessageInstance &   session );
			void clear(){
				std::lock_guard<std::mutex> guard(session_mutex_);
				sessions_.clear();
			}

			MessageInstance at(std::size_t index);
			
			std::size_t size(){
				std::lock_guard<std::mutex> guard(session_mutex_);
				return sessions_.size();
			}
			int unique_id(){ 
				std::lock_guard<std::mutex> guard(session_mutex_);
				return ++unique_id_;
			}
		private:
			std::size_t max_num_;
			int unique_id_;
			std::set<MessageInstance> sessions_;
			std::mutex session_mutex_;
		};
		
		template<typename MessageInstance>
		void Container<MessageInstance>::push(const MessageInstance & session ){
			std::lock_guard<std::mutex> guard(session_mutex_);
			if( !max_num_ || sessions_.size() < max_num_ )
				sessions_.insert( session );
			else
				std::cerr<<"===> There are too many connections, allowed connections:"<<max_num_<<std::endl;
		}

		template<typename MessageInstance>
		void Container<MessageInstance>::pop( const MessageInstance & session ){
			std::lock_guard<std::mutex> guard(session_mutex_);
			auto it = sessions_.find( session );
			if( it != sessions_.end() )
				sessions_.erase( it );
		}


		template<typename MessageInstance>
		MessageInstance Container<MessageInstance>::at(std::size_t index){
			std::lock_guard<std::mutex> guard(session_mutex_);
			auto front_interator = sessions_.begin();
			std::size_t i =0;
			while( i < index && front_interator != sessions_.end()){
				++i;++front_interator;
			}
			if( front_interator == sessions_.end() )
				return MessageInstance();
			return (*front_interator);
		}


}//namespace Message
}//namespace Shannon

#endif
