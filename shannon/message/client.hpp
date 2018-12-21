///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
/***************************************************************************
**                                                                        **
**					A simple template message client class                **
**       Can be easily used in intra-system message communications        **
**      Must work in coordination with a corresponding message server     **
**                                                                        **
****************************************************************************/

#ifndef SHANNON_MESSAGE_CLIENT_HPP
#define SHANNON_MESSAGE_CLIENT_HPP


#include <boost/mpl/vector.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/mpl/replace_if.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/find.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/back_inserter.hpp>
#include "client_sequence_impl.hpp"

namespace Shannon{
	namespace Message{

		////T is a type
	template<typename T, bool is_sequence>
	struct ClientImpl{
		/*!
			if T is a void type, then we use ClientBaseSync (only send message)
			otherwise, we use ClientSequenceImpl< mpl::vector<T> >
		*/
		typedef typename boost::mpl::if_< boost::is_same<T, void>, 
										ClientBaseSync,
										ClientSequenceImpl<boost::mpl::vector<T> >
								>::type type;
	};

	/// T is a mpl::vector
	template<typename T>
	struct ClientImpl<T, true>{
		/*!
			we will first remove all void type in T, the result is void_free_T,
				if void_free_T is empty, then we use ClientBaseSync (only send message)
				otherwise, we use ClientSequenceImpl<void_free_T>
		*/
		typedef typename boost::mpl::remove_if< 
							T,
							typename boost::mpl::lambda< boost::is_same<void, boost::mpl::_1> >::type,
							boost::mpl::back_inserter< boost::mpl::vector<> >
						>::type void_free_T;

		typedef typename boost::mpl::if_< boost::mpl::empty<void_free_T>, 
								ClientBaseSync,
								ClientSequenceImpl<void_free_T> 
						>::type type;

	};

	template<typename T>
	struct Client{
		typedef typename ClientImpl<T, boost::mpl::is_sequence<T>::value >::type type;

		/*!
		typedef typename boost::mpl::if_< boost::mpl::is_sequence<T>, 
										ClientSequenceImpl<T>,
											typename boost::mpl::if_< boost::is_same<T, void>, ClientBase,
													ClientSequenceImpl<boost::mpl::vector<T> >
											>::type
										>::type type;
			*/
	};

}//namespace Message
}//namespace Shannon

#endif