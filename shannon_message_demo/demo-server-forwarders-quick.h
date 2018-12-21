///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef DEMO_SHANNON_MESSAGE_QUICK_FORWARDER_H
#define DEMO_SHANNON_MESSAGE_QUICK_FORWARDER_H

#include <iostream>
#include <set>
/// We need BOOST_REGEX_NO_LIB here, see https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/using.html
#define BOOST_REGEX_NO_LIB
#include <shannon/message/helper/subscribe_quick_forwarder.hpp>
#include "demo-message-datastruct.h"



template<typename DataStruct>
class DefaultQuickDirectForwarder : public Shannon::Message::QuickForwarder<DataStruct>
{
public:
	DefaultQuickDirectForwarder(const Shannon::Message::MessageTypeGroup & messageGroup,  const std::vector<std::string> & keys )
		:Shannon::Message::QuickForwarder<DataStruct>( messageGroup ) 
	{
		this->observer_keys_.insert( this->observer_keys_.begin(), keys.begin(), keys.end() );
	}

};

typedef DefaultQuickDirectForwarder<TestSTructFirst>	TestFirstQuickForwarder;
typedef DefaultQuickDirectForwarder<TestSTructSecond> TestSecondQuickForwarder;


#endif