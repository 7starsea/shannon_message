///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
/***************************************************************************
**                                                                        **
**					A simple template message server class                **
**       Can be easily used in intra-system message communications        **
**      Must work in coordination with a corresponding message client     **
**                                                                        **
****************************************************************************/
#ifndef SHANNON_MESSAGE_SERVER_HPP
#define SHANNON_MESSAGE_SERVER_HPP
#include <boost/mpl/is_sequence.hpp>

#include "server_sequence_impl.hpp"

namespace Shannon{
	namespace Message{


template<typename T>
struct Server{

	typedef typename boost::mpl::if_< boost::mpl::is_sequence<T>, 
									ServerSequenceImpl<T>,
									ServerSequenceImpl<boost::mpl::vector<T>>
									>::type type;

};


}//namespace Message
}//namespace Shannon



#endif