///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#include "binary_logger_util.h"
#include <sys/stat.h>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

/////			%LoggerName %LoggerType(Record/Last) %LoggerDate %LoggerFileFormat(csv/txt/bin)
boost::format BinaryLoggerUtil::m_logger_filename_format( "DataCenter.%s.%s.%s.%s" ); 
int BinaryLoggerUtil::freq_in_sec_ = 2;
std::vector< std::function< void(void) >  > BinaryLoggerUtil::functions_;

void BinaryLoggerUtil::append( std::function< void(void) > f )
{
	functions_.push_back( f );
}


void BinaryLoggerUtil::run()
{
	boost::thread tLoggerThread(  BinaryLoggerUtil::__run  );
	tLoggerThread.detach();
}

void BinaryLoggerUtil::__run()
{

	while(true)
	{
		unsigned int l = functions_.size();
		for(unsigned int i = 0; i < l ; i++)
		{
			functions_[i] ();
		}
		boost::this_thread::sleep(boost::posix_time::milliseconds( freq_in_sec_  * 1000 ));
	}
}


void BinaryLoggerUtil::SetWriterFrequency( int freq_in_sec )
{
	freq_in_sec_ = freq_in_sec;
}


long BinaryLoggerUtil::filesize(const std::string & filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}
bool BinaryLoggerUtil::open_bin_file( std::fstream & ofs, const std::string & filename_bin, std::ios_base::openmode mode )
{
	if( ofs.is_open() )	ofs.close();

	bool isOpened = true;

	ofs.open(filename_bin.c_str(), std::fstream::in | std::fstream::out | std::fstream::binary | mode);
    if( ofs.fail() )
    {
        ofs.clear();
        ofs.open( filename_bin.c_str(), std::fstream::out );
        ofs.close();
        ofs.open( filename_bin.c_str(), std::fstream::in | std::fstream::out |std::fstream::binary | mode);
    }
   if (ofs.fail()) {
        std::cout<<"=========> Can not open file:"<<filename_bin<<" <=============="<<std::endl;
		isOpened = false;
        ofs.clear();
    }
   return isOpened;
}