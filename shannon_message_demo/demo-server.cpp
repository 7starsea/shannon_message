///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#include <iostream>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>

/// We need BOOST_REGEX_NO_LIB here, see https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/using.html
#define BOOST_REGEX_NO_LIB
#include <shannon/message/server.hpp>

#include "demo-server-forwarders.h"
#include "binary_logger_util.h"
void send_data_loop(TestFirstForwarder * fst_forwarder , TestSecondForwarder * snd_forwarder ){

	boost::random::mt19937 gen((unsigned int)time(0));
	boost::normal_distribution<> nd(0, 1);///mean, std variance
	boost::uniform_01 <> un;
	std::vector<std::string> ids;
	ids.push_back("10000585"); ids.push_back("10000586"); ids.push_back("10000575"); ids.push_back("10000576"); ids.push_back("10000577"); ids.push_back("10000578"); ids.push_back("10000579"); ids.push_back("10000589"); ids.push_back("10000593"); ids.push_back("10000597");
	ids.push_back("10000601"); ids.push_back("10000602"); ids.push_back("10000603"); ids.push_back("10000604"); ids.push_back("10000605");
	ids.push_back("10000567"); ids.push_back("10000551"); ids.push_back("10000552"); ids.push_back("10000541"); ids.push_back("10000535"); ids.push_back("10000529"); ids.push_back("10000523"); ids.push_back("10000459"); ids.push_back("10000449"); ids.push_back("10000450"); ids.push_back("10000451"); ids.push_back("10000452"); ids.push_back("10000453"); ids.push_back("10000463"); ids.push_back("10000464"); ids.push_back("10000469"); ids.push_back("10000473"); ids.push_back("10000477");
	ids.push_back("10000569"); ids.push_back("10000555"); ids.push_back("10000556"); ids.push_back("10000557"); ids.push_back("10000558"); ids.push_back("10000559"); ids.push_back("10000571"); ids.push_back("10000573"); ids.push_back("10000591"); ids.push_back("10000595"); ids.push_back("10000599"); 
	ids.push_back("10000587"); ids.push_back("10000588"); ids.push_back("10000580"); ids.push_back("10000581"); ids.push_back("10000582"); ids.push_back("10000583"); ids.push_back("10000584"); ids.push_back("10000590"); ids.push_back("10000594"); ids.push_back("10000598"); ids.push_back("10000606"); ids.push_back("10000607"); ids.push_back("10000608"); ids.push_back("10000609"); ids.push_back("10000610"); ids.push_back("10000568"); ids.push_back("10000553"); ids.push_back("10000554"); ids.push_back("10000542"); ids.push_back("10000536"); ids.push_back("10000530"); ids.push_back("10000524"); ids.push_back("10000460"); ids.push_back("10000454"); ids.push_back("10000455"); ids.push_back("10000456"); ids.push_back("10000457"); ids.push_back("10000458"); ids.push_back("10000465"); ids.push_back("10000466"); ids.push_back("10000470"); ids.push_back("10000474"); ids.push_back("10000478"); ids.push_back("10000570"); ids.push_back("10000560"); ids.push_back("10000561"); ids.push_back("10000562"); ids.push_back("10000563"); ids.push_back("10000564"); ids.push_back("10000572"); ids.push_back("10000574"); ids.push_back("10000592"); ids.push_back("10000596"); ids.push_back("10000600"); 
	int index = 0;
	int hours = 9, minutes = 30, seconds = 0;

	TestSTructFirst fst;
	TestSTructSecond snd;
	memset( &fst, 0, sizeof( fst ));
	memset( &snd, 0, sizeof( snd ));

	fst.value1 = 0.5;
	fst.value2 = 0.25;
	fst.value3 = 0.1244;
	fst.value4 = 0.1240;

	snd.value1 = 0.5;
	snd.value2 = 0.25;
	snd.value3 = 0.1244;
	snd.value4 = 0.1240;


	while( true ){
		sprintf(fst.update_time, "%02d:%02d:%02d", hours, minutes, seconds);
		sprintf(snd.update_time, "%02d:%02d:%02d", hours, minutes, seconds);

		double tn = nd(gen), tu = un(gen), tn2 = nd(gen), tu2 = un(gen);
		double t = std::exp(0.0001 + 0.003  * tn  );

	
		fst.value1 += 0.0001 * tn;
		fst.value2 += tn > 1 ? tu/1000 - 0.0005 : 0;
		fst.value3 += tn > 1 ? tu/1000 - 0.0005 : 0;
		fst.value4 += tn > 1 ? tu/1000 - 0.0005 : 0;

		snd.value1	+= 0.001 * tn + 0.0001 * tn2;
		snd.value2+= 0.001 * (tn > 2 ? 1 : ( tn < 2 ? -1 : 0.1) );
		snd.value3 +=0.001 * (tn > 2 ? 1 : ( tn < 2 ? -1 : 0.1) );
		snd.value4	+= 0.001 * (tn > 2 ? 1 : ( tn < 2 ? -1 : 0.1) );

		int sent = 0, total_send = 10000;
		for(auto it = ids.begin(); it != ids.end(); it++ ){
			strcpy( fst.id, (*it).c_str() );
			strcpy( snd.id, (*it).c_str() );

			fst_forwarder->DirectForward( fst );
			snd_forwarder->DirectForward( snd );
			++sent;
			if( sent >= total_send ) break;
		}

		++index;
		if( index % 10 == 0) std::cout<<"Send:"<<index<<std::endl;

		boost::this_thread::sleep(boost::posix_time::milliseconds( 1000 ));

		seconds += 10;
		minutes += seconds/60;	seconds = seconds%60;
		hours += minutes/60;	minutes = minutes%60;
		if( hours == 11 && minutes >= 30 ){
			hours = 13; minutes = 0;
		}else if( hours >= 15 ){
			std::cout<<"Send Message Finished!"<<std::endl;
			break;
		}
	}
}

int main()
{
	 BinaryLogger<TestSTructSecond> snd_logger("TestSTructSecond", false, true, std::fstream::trunc);
	 BinaryLogger<TestSTructFirst> fst_logger("TestSTructFirst", false, true, std::fstream::trunc);


	std::vector<std::string> ids;
	ids.push_back("10000585"); ids.push_back("10000586"); ids.push_back("10000575"); ids.push_back("10000576"); ids.push_back("10000577"); ids.push_back("10000578"); ids.push_back("10000579"); ids.push_back("10000589"); ids.push_back("10000593"); ids.push_back("10000597"); ids.push_back("10000601"); ids.push_back("10000602"); ids.push_back("10000603"); ids.push_back("10000604"); ids.push_back("10000605"); ids.push_back("10000567"); ids.push_back("10000551"); ids.push_back("10000552"); ids.push_back("10000541"); ids.push_back("10000535"); ids.push_back("10000529"); ids.push_back("10000523"); ids.push_back("10000459"); ids.push_back("10000449"); ids.push_back("10000450"); ids.push_back("10000451"); ids.push_back("10000452"); ids.push_back("10000453"); ids.push_back("10000463"); ids.push_back("10000464"); ids.push_back("10000469"); ids.push_back("10000473"); ids.push_back("10000477"); ids.push_back("10000569"); ids.push_back("10000555"); ids.push_back("10000556"); ids.push_back("10000557"); ids.push_back("10000558"); ids.push_back("10000559"); ids.push_back("10000571"); ids.push_back("10000573"); ids.push_back("10000591"); ids.push_back("10000595"); ids.push_back("10000599"); ids.push_back("10000587"); ids.push_back("10000588"); ids.push_back("10000580"); ids.push_back("10000581"); ids.push_back("10000582"); ids.push_back("10000583"); ids.push_back("10000584"); ids.push_back("10000590"); ids.push_back("10000594"); ids.push_back("10000598"); ids.push_back("10000606"); ids.push_back("10000607"); ids.push_back("10000608"); ids.push_back("10000609"); ids.push_back("10000610"); ids.push_back("10000568"); ids.push_back("10000553"); ids.push_back("10000554"); ids.push_back("10000542"); ids.push_back("10000536"); ids.push_back("10000530"); ids.push_back("10000524"); ids.push_back("10000460"); ids.push_back("10000454"); ids.push_back("10000455"); ids.push_back("10000456"); ids.push_back("10000457"); ids.push_back("10000458"); ids.push_back("10000465"); ids.push_back("10000466"); ids.push_back("10000470"); ids.push_back("10000474"); ids.push_back("10000478"); ids.push_back("10000570"); ids.push_back("10000560"); ids.push_back("10000561"); ids.push_back("10000562"); ids.push_back("10000563"); ids.push_back("10000564"); ids.push_back("10000572"); ids.push_back("10000574"); ids.push_back("10000592"); ids.push_back("10000596"); ids.push_back("10000600"); 

	Shannon::Message::MessageTypeGroup messageLoop;
	messageLoop.subscribe	= 201;
	messageLoop.unsubscribe = 202;
	messageLoop.update		= 203;
	TestFirstForwarder fst_forwarder(messageLoop, &fst_logger, ids);
	fst_forwarder.Init("");

	messageLoop.subscribe	= 211;
	messageLoop.unsubscribe = 212;
	messageLoop.update		= 213;
	TestSecondForwarder snd_forwarder(messageLoop, &snd_logger, ids);
	snd_forwarder.Init("");

	boost::thread tLoggerThread(  boost::bind( &send_data_loop, &fst_forwarder, &snd_forwarder)  );
	tLoggerThread.detach();

	BinaryLoggerUtil::run();
	
	typedef Shannon::Message::Server<Shannon::Message::SubscribeField>::type MySubscribeServer;

	MySubscribeServer server( 4444 );
	///зЂВс  Observer [Forwarder is a subclass of Observer]
	server.Register<Shannon::Message::SubscribeField>( &fst_forwarder );
	server.Register<Shannon::Message::SubscribeField>( &snd_forwarder );

	std::cout<<"The Shannon::Message Subscribe Server starts:"<<std::endl;
	server.run();

	while(true){ boost::this_thread::sleep(boost::posix_time::milliseconds(1000 * 100 )); }; 

}