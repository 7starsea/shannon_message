///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef SHANNON_MESSAGE_HELPER_CONSOLE_DATA_STRUCT_H
#define SHANNON_MESSAGE_HELPER_CONSOLE_DATA_STRUCT_H
#include <string.h>
#include <shannon/message/data_type.h>

#define SHANNON_MESSAGE_CONSOLE_FEEDBACK_MSG_TYPE 99991


namespace Shannon{
	namespace Message{


	//配置信息订阅结构
	typedef struct ConsoleSubscribeField{
		char id[31];
		MessageResumeType resume_type;

		char client_name[31];	////Quoter.50ETF, Covariance.50ETF, Hedger.50ETF
		char client_type;	////Quoter = Q, Covariance = C, Generator = G, Operator = O, Hedger = H
		ConsoleSubscribeField(){
			memset(this, 0, sizeof(ConsoleSubscribeField));
		}
		const char * key_id()const{
			return this->id;
		}
	}ConsoleSubscribeField;

	///数据发送结构，增加is_last选项
	template<typename DataStruct>
	struct ConsoleDataStruct{
		DataStruct data;
		bool is_last;
		ConsoleDataStruct()
			:data(), is_last(false){}

		ConsoleDataStruct(const DataStruct & tData)
			:data(tData), is_last(false){}

		const char * key_id(){
			return data.key_id();
		}
	};

	//feedback message STRUCT
	typedef struct ConsoleFeedbackField{
		char id[31];
		
		char message[512];		////useful 

		bool is_error;		////Information Message or Error Message
		ConsoleFeedbackField(){
			memset(this, 0, sizeof(ConsoleFeedbackField));
		}
		const char * key_id()const{
			return this->id;
		}
	}ConsoleFeedbackField;

	template <typename T, int N>
	struct ConsoleDataStructWrap{
		typedef T type;
		const static int value = N;
	};
}//namespace Message
}//namespace Shannon

#endif