///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#include <fstream>
#include <sys/stat.h>
#include <boost/lexical_cast.hpp> 
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/vector.hpp>

#include <vector>
#include <algorithm>
#include <functional>

#include <algorithm>


#include <shannon/message/helper/console_client_initializer.hpp>


#include "demo-console-datastruct.h"


#define Galois_W Shannon::Message::ConsoleDataStructWrap

typedef boost::mpl::vector<
	Galois_W<QuoterOptionConfig, 1>,
	Galois_W<QuoterSystemConfig, 2>,
	Galois_W<GeneratorSystemConfig, 6>,
	Galois_W<GeneratorOptionConfig, 8>,
	Galois_W<GeneratorExpiryMonthConfig, 9>
			> MyConsoleClientInitDataStructs;
typedef Shannon::Message::ConsoleClientInitializer< MyConsoleClientInitDataStructs >	MyConsoleClientInitializer;

#undef Galois_W

//实现一个打印程序运行消息的DemoLogger
void DemoLogger(int data_received, const std::string & msg)
{
	std::cout << "Demo msg content: " << msg <<"\n Connected? "<< (data_received>0?"Yes":"No") <<" DataReceived: "<<( data_received>0 ? data_received-1 : -data_received-1 ) << std::endl;
}

//实现一个基本的DemoParams的更新函数，只在标准输出流中打印收到的参数
struct MyConsoleDemoUpdater{
	MyConsoleDemoUpdater( MyConsoleClientInitializer * my_console, const std::string & subscribe_key)
		:my_console_(my_console), subscribe_key_(subscribe_key){}

	void OnReceiveQuoterOptionConfig(QuoterOptionConfig * demo_params){
		std::cout<< "Received ID: " << demo_params->option_id << ", tolGreater:" << demo_params->tolerance_greater << std::endl;

		if( demo_params->tolerance_greater > 1 ){  ////security check 
			Shannon::Message::ConsoleFeedbackField feedback;
			memset(&feedback,0,sizeof(feedback));
			feedback.is_error = true;

			strncpy(feedback.id, subscribe_key_.c_str(), sizeof(feedback.id)-1);
			strncpy(feedback.message, "QuoterOptionConfig.tolerance_greater did not pass security check!", sizeof(feedback.message)-1);
			//////Send Feedback message
			my_console_->SendFeedbackMsg(feedback);

		}
	}
	void OnReceiveQuoterSystemConfig( QuoterSystemConfig * demo_param ){
	}

	void OnReceiveGeneratorOptionConfig(GeneratorOptionConfig * demo_params){
		std::cout<< "ID: " << demo_params->option_id << ", risk rate:" << demo_params->init_calibrated_sigma << std::endl;

		if( demo_params->init_calibrated_sigma < 0 ){  ////security check 
			Shannon::Message::ConsoleFeedbackField feedback;
			memset(&feedback,0,sizeof(feedback));
			feedback.is_error = true;

			strncpy(feedback.id, subscribe_key_.c_str(), sizeof(feedback.id)-1);
			strncpy(feedback.message, "GeneratorOptionConfig.riskfree_rate did not pass security check!", sizeof(feedback.message)-1);
			my_console_->SendFeedbackMsg(feedback);
		}
	}

	void OnReceiveGeneratorSystemConfig(GeneratorSystemConfig * demo_params){

	}


private:
	MyConsoleClientInitializer * my_console_;
	const std::string subscribe_key_;
};




int main()
{

	const std::string console_address = "127.0.0.1:8011";
	std::vector<std::string> subscribe_keys;

	MyConsoleClientInitializer * my_console_initializer = new MyConsoleClientInitializer(console_address, DemoLogger);
	my_console_initializer->SetClientName('Q', "Quoter.m");


	subscribe_keys.push_back("m_Quoter1709");
	subscribe_keys.push_back("m_Quoter1801");
	my_console_initializer->Register<QuoterOptionConfig>(subscribe_keys,  SHANNON_MESSAGE_RESUME_RESTART);

	subscribe_keys.resize(0);
	subscribe_keys.push_back("m1709");
	my_console_initializer->Register<QuoterSystemConfig>(subscribe_keys,  SHANNON_MESSAGE_RESUME_RESTART);

	my_console_initializer->start();

///	my_console_initializer->stop();
	
	
	std::cout<<"===> Waiting for reading console configuration..."<<std::endl;
	while( !my_console_initializer->IsInit() ){
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	}
	std::cout<<"===> Finished reading console configuration!"<<std::endl;
	
//	std::map<std::string, GeneratorOptionConfig> generator_option_config_map = my_console_initializer->RtnInitData<GeneratorOptionConfig>( );
//	std::map<std::string, GeneratorSystemConfig> generator_system_config_map = my_console_initializer->RtnInitData<GeneratorSystemConfig>( );

	std::map<std::string, QuoterOptionConfig> quoter_option_config_map = my_console_initializer->RtnInitData<QuoterOptionConfig>( );
	std::map<std::string, QuoterSystemConfig> quoter_system_config_map = my_console_initializer->RtnInitData<QuoterSystemConfig>();

	/**********

		

		Your specific initialization, possible including (check all parameters from console)
			
			





	***********/

	///注册更新回调函数
	MyConsoleDemoUpdater updater(my_console_initializer, "510050");

//	my_console_initializer->RegisterUpdater<GeneratorOptionConfig>(  std::bind(&MyConsoleDemoUpdater::OnReceiveGeneratorOptionConfig, &updater, std::placeholders::_1) );
//	my_console_initializer->RegisterUpdater<GeneratorSystemConfig>(  std::bind(&MyConsoleDemoUpdater::OnReceiveGeneratorSystemConfig, &updater, std::placeholders::_1) );
	my_console_initializer->RegisterUpdater<QuoterOptionConfig>( std::bind(&MyConsoleDemoUpdater::OnReceiveQuoterOptionConfig, &updater, std::placeholders::_1) );
	my_console_initializer->RegisterUpdater<QuoterSystemConfig>(  std::bind(&MyConsoleDemoUpdater::OnReceiveQuoterSystemConfig, &updater, std::placeholders::_1) );


	
	while(true){ 
		boost::this_thread::sleep(boost::posix_time::milliseconds(2000 * 100)); 
	}
	
	/// For safely exit the program, please add the following line at some point;
	Shannon::Message::ClientBase::safe_exit();
	return 0;
}