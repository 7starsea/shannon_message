///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef DEMO_SHANNON_MESSAGE_FORWARDER_H
#define DEMO_SHANNON_MESSAGE_FORWARDER_H

#include <iostream>

/// We need BOOST_REGEX_NO_LIB here, see https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/using.html
#define BOOST_REGEX_NO_LIB
#include <shannon/message/helper/subscribe_forwarder.hpp>

#include "binary_logger.hpp"
#include "demo-message-datastruct.h"


template<typename DataStruct>
class DefaultDirectForwarder : public Shannon::Message::Forwarder<DataStruct, BinaryLogger<DataStruct>, true>
{
public:
	DefaultDirectForwarder(const Shannon::Message::MessageTypeGroup & messageGroup, BinaryLogger<DataStruct> * logger, const std::vector<std::string> & keys )
		:Shannon::Message::Forwarder<DataStruct, BinaryLogger<DataStruct>, true>(messageGroup, logger)
	{
		this->observer_keys_.insert( this->observer_keys_.begin(), keys.begin(), keys.end() );
	}
};
	

typedef DefaultDirectForwarder<TestSTructFirst> TestFirstForwarder;
typedef DefaultDirectForwarder<TestSTructSecond> TestSecondForwarder;


#endif