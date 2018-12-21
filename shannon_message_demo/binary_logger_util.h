///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef BinaryLoggerUtil_Header_Observer
#define BinaryLoggerUtil_Header_Observer
#include <iostream>
#include <string>
#include <fstream>
#include <boost/format.hpp>

class BinaryLoggerUtil
{
public:
	static	boost::format m_logger_filename_format; 
/////	%LoggerName %LoggerType(Record/Last) %LoggerDate %LoggerFileFormat(csv/txt/bin)


	static void SetWriterFrequency( int freq_in_sec );
	static void append( std::function< void(void) > );
	/////// must be called after all BinaryLogger has been init
	static void run();

	static long filesize(const std::string & filename);
	static bool open_bin_file( std::fstream & ofs, const std::string & filename_bin, std::ios_base::openmode mode );
private:
	static int freq_in_sec_;
	static std::vector< std::function< void(void) >  > functions_;


	static void __run();
};
#endif
