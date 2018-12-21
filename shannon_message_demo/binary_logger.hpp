///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef BinaryLogger_Header_Observer
#define BinaryLogger_Header_Observer

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
#include <mutex>
//#include <boost/thread/shared_mutex.hpp>
#include <functional>
#include <cstdio> ////using std::remove function to remove a file / std::rename function to rename a file


#include <boost/mpl/bool.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
//#include <boost/date_time/posix_time/posix_time.hpp>


#include "binary_logger_util.h"

template<typename DataStruct>
class BinaryLogger
{
public:
	////online
    BinaryLogger(const std::string & name,const bool record_last, bool auto_write, 
		std::ios_base::openmode mode = std::fstream::trunc,	const std::string & date = ""
		);
	~BinaryLogger();
    
    void append(const DataStruct &data);
	void append(const std::vector<DataStruct> &data);
	void readFromFirstCache(std::vector<DataStruct> & vData, const bool writeToFile =true );
    void readFromSecondCache(std::vector<DataStruct> & vData, unsigned int startpos, unsigned int deviation, unsigned int count = 0);
	void enableAutoWriteToSecondCache(bool);

	void autoWriteToSecondCache();

	////return value: False means empty file; true means non-empty file
	bool recover( const int dataStructsPerBlock );

	void appendLast(const std::vector<DataStruct> &vData  );
	void readLast( std::vector<DataStruct> &vData, const bool IsYdData=true );


public:
    const std::string m_logger_name;
private:
	const bool m_record_last;

	int m_data_structs_per_block;
	unsigned int m_data_written_count;
	bool m_auto_write;
	
	std::ios_base::openmode m_openmode;
	boost::gregorian::date m_date;

	std::string m_record_filename_txt;
	std::string m_last_filename_txt;

	std::string m_record_filename_bin;
	std::string m_last_filename_bin;

	std::fstream m_record_ofs;
	std::fstream m_last_ofs;
    std::vector<DataStruct> m_pending_data;

	std::mutex m_data_mutex;
    std::mutex m_file_mutex;
	std::mutex m_flag_mutex;

};


////online
template<typename DataStruct>
BinaryLogger<DataStruct>::BinaryLogger(const std::string & name, const bool record_last, bool auto_write, 
		std::ios_base::openmode mode/* = std::fstream::trunc*/,		const std::string & date 
		)
	:m_logger_name(name),m_record_last(record_last), m_data_structs_per_block(0),m_data_written_count(0),	
	 m_auto_write(auto_write), m_openmode(mode)
{
	if( date.size() >=8 )
	{
		m_date = boost::gregorian::date_from_iso_string( date );
	}else{
		m_date = boost::gregorian::day_clock::local_day();
	}

	std::string t = boost::gregorian::to_iso_string( m_date );

	BinaryLoggerUtil::m_logger_filename_format.clear();
	BinaryLoggerUtil::m_logger_filename_format%name%"Record"%t%"bin";
	m_record_filename_bin = BinaryLoggerUtil::m_logger_filename_format.str();

	boost::filesystem::path cur_path( boost::filesystem::current_path() );
	std::cout<<"DataCenter Files Directory:"<<cur_path<<std::endl;

	cur_path /= m_record_filename_bin;
	m_record_filename_bin = cur_path.string();

	if( m_record_last )
	{
		BinaryLoggerUtil::m_logger_filename_format.clear();
		BinaryLoggerUtil::m_logger_filename_format%name%"Last"%t%"bin";
		m_last_filename_bin = BinaryLoggerUtil::m_logger_filename_format.str();

		cur_path = boost::filesystem::current_path() ;
		cur_path /= m_last_filename_bin;
		m_last_filename_bin = cur_path.string();
	}

//	if( ! boost::filesystem::exists( m_record_filename_bin.c_str() ) )
//	{
//		std::cout<<"The Record File:"<<m_record_filename_bin<<" Does Not Exist!"<<std::endl;
//	}

	if( ! BinaryLoggerUtil::open_bin_file(m_record_ofs, m_record_filename_bin,  m_openmode ) )
		std::cout<<"Could not open record file:"<<m_record_filename_bin<<std::endl;
//	if( !__open_bin_file(m_last_ofs, m_last_filename_bin ) )
//		std::cout<<"Could not open last file:"<<m_last_filename_bin<<std::endl;



	if( m_auto_write )
	{
		BinaryLoggerUtil::append( std::bind( &BinaryLogger<DataStruct>::autoWriteToSecondCache, this ) );	
	}
};

template<typename DataStruct>
BinaryLogger<DataStruct>::~BinaryLogger()
{	
	std::unique_lock<std::mutex> writeFileLock(m_file_mutex);
	if( !m_record_ofs.is_open() ) return;

	m_record_ofs.close();
	m_last_ofs.close();
};


template<typename DataStruct>
void BinaryLogger<DataStruct>::appendLast(const std::vector<DataStruct> &vData  ){

	if( m_record_last )
	{
		std::ofstream ofs( m_last_filename_bin.c_str(),  std::ios::out |std::ios::binary | std::ios::trunc );
		if( ofs.is_open()  ){
			for(auto it = vData.begin() ; it!= vData.end(); it++ ){
				ofs.write((const char *)&(*it), sizeof(DataStruct) );
				if( !ofs.good() ){
					std::cerr <<"In BinaryLogger: fail to write last record field to file:"<<m_last_filename_bin <<std::endl;
					break;
				}
			}
			ofs.close();
		}else{
			std::cout<<"===> Could not open file:"<<m_last_filename_bin<<std::endl;
		}
	}
}

////Read Yd Data
template<typename DataStruct>
void BinaryLogger<DataStruct>::readLast( std::vector<DataStruct> &vData, const bool IsYdData /*=true*/  ){

	if( m_record_last )
	{
		boost::gregorian::date tmpDate = m_date ;
		if( IsYdData )	tmpDate -=  boost::gregorian::days( tmpDate.day_of_week() == boost::date_time::Monday ? 3 : 1 );
		std::string t = boost::gregorian::to_iso_string( tmpDate );

		BinaryLoggerUtil::m_logger_filename_format.clear();
		BinaryLoggerUtil::m_logger_filename_format%m_logger_name%"Last"%t%"bin";
		t = BinaryLoggerUtil::m_logger_filename_format.str();

		boost::filesystem::path cur_path( boost::filesystem::current_path() );
		cur_path /= t;
		t = cur_path.string();

		if( boost::filesystem::exists( t.c_str() ) )
		{
			std::ifstream ofs( t.c_str(), std::ios::in  | std::ios::binary );
			if( ofs.is_open() ){
				DataStruct data;
				while (ofs.peek() != EOF ) {
					ofs.read( (char *)&data, sizeof(DataStruct));
					vData.push_back( data );
				}
				ofs.close();
			}
		}else{
			std::cout<<"===> The file:"<<t<<" does not exists!"<<std::endl;
		}
	}
}


////return value: False means empty file; true means non-empty file
template<typename DataStruct>
bool BinaryLogger<DataStruct>::recover(const int dataStructsPerBlock)
{
	std::unique_lock<std::mutex> writeFileLock(m_file_mutex);
	m_data_structs_per_block = dataStructsPerBlock;
//   if( !m_record_ofs.is_open() ) return false;
	bool emptyFile = false;
	long fileSize = BinaryLoggerUtil::filesize( m_record_filename_bin );
	if( fileSize > 0 ){
		m_data_written_count = fileSize/(sizeof(DataStruct)*dataStructsPerBlock);
		long reminder = fileSize%(sizeof(DataStruct)*dataStructsPerBlock);
		if( reminder > 0 ){
			////// Delete the un chunked data
			m_record_ofs.close();
			std::string tmp = m_record_filename_bin + ".tmp";
			int rc = std::rename( m_record_filename_bin.c_str(), tmp.c_str() );
			BinaryLoggerUtil::open_bin_file(m_record_ofs, m_record_filename_bin,  std::fstream::trunc );
			if( 0 == rc  )
			{
				std::ifstream ofs( tmp.c_str(), std::ios::in  | std::ios::binary );
				if( m_record_ofs.is_open() && ofs.is_open() ){
					long count = fileSize/(sizeof(DataStruct)*dataStructsPerBlock), i = 0;
					count *= dataStructsPerBlock;
					DataStruct data;
					while( i < count && ofs.peek() != EOF ){
						ofs.read( (char *)&data, sizeof(DataStruct));
						m_record_ofs.write((const char *)&data, sizeof(DataStruct) );
						if( !m_record_ofs.good() ){
							std::cerr <<"===> In BinaryLogger: fail to recover record field to file:"<<m_record_filename_bin <<std::endl;
							break;
						}
						i++;
					}
					if( i < count ){
						std::cerr<<"===> In BinaryLogger: fail to recover all record field to file:"<<m_record_filename_bin <<". Recovered: "<< i <<"/"<<count<<std::endl;
					}
					ofs.close();
					std::remove( tmp.c_str() );
				}else{
					emptyFile = true;
					std::cout<<"===> Recover Old Data Failed b/c could not open file:"<<m_record_filename_bin<<" or "<< tmp <<std::endl;
				}
			}else{
				emptyFile = true;
				std::cout<<"===> Recover Old Data Failed b/c could not rename file:"<<m_record_filename_bin<<std::endl;
			}
		}
	}else{
		emptyFile = true;
	}
	return emptyFile;
}

template<typename DataStruct>
void BinaryLogger<DataStruct>::append(const DataStruct &data )
{
	std::unique_lock<std::mutex> writeFileLock(m_data_mutex);
	m_pending_data.push_back( data ); 	
};

template<typename DataStruct>
void BinaryLogger<DataStruct>::append(const std::vector<DataStruct> &data )
{
	std::unique_lock<std::mutex> writeFileLock(m_data_mutex);
	m_pending_data.insert( m_pending_data.end(), data.begin(), data.end() );
};

template<typename DataStruct>
void BinaryLogger<DataStruct>::enableAutoWriteToSecondCache( bool auto_write )
{

	std::unique_lock<std::mutex> writeFileLock(m_flag_mutex);
	m_auto_write = auto_write;
};
template<typename DataStruct>
void BinaryLogger<DataStruct>::autoWriteToSecondCache()
{
	std::unique_lock<std::mutex> writeFileLock(m_flag_mutex);
	if( m_auto_write ){
		std::vector<DataStruct>  vData;
		this->readFromFirstCache( vData );
	}
};


template<typename DataStruct>
void BinaryLogger<DataStruct>::readFromFirstCache(std::vector<DataStruct> & vData, const bool writeToFile /*=true*/)
{
	unsigned int beg = vData.size();
	{
		std::unique_lock<std::mutex> writeFileLock(m_data_mutex);
		vData.reserve( vData.size() + m_pending_data.size() );
		vData.insert( vData.begin(), m_pending_data.begin(), m_pending_data.end() );
		if (writeToFile )m_pending_data.resize(0);
	}
	
	if( writeToFile ){
		std::unique_lock<std::mutex> writeFileLock(m_file_mutex);
		if( !m_record_ofs.is_open() ) return;
		m_record_ofs.seekp(0, std::ios_base::end);
//		for (auto it=vData.begin(); it!=vData.end(); it++) {
		unsigned int end = vData.size();
		for(unsigned int i = beg; i< end; i++){
			m_record_ofs.write((const char *)&( vData[i] ), sizeof(DataStruct) );
			if( !m_record_ofs.good() ){
				std::cerr <<"===> In BinaryLogger: fail to write record field to file:"<<m_record_filename_bin <<std::endl;
				break;
			}
			m_data_written_count++;
		}
		m_record_ofs.flush();
	}
};
template<typename DataStruct>
void BinaryLogger<DataStruct>::readFromSecondCache(std::vector<DataStruct> & vData, unsigned int startpos, unsigned int deviation, unsigned int count)
{
//	boost::unique_lock<boost::shared_mutex> writeFileLock(m_file_mutex);
	std::unique_lock<std::mutex> writeFileLock(m_file_mutex);
	if( !m_record_ofs.is_open() ) return;
//	long fileSize = BinaryLoggerUtil::filesize( m_record_filename_bin );
	if( m_data_written_count && deviation ){
		if( !count ){
			count = 1+m_data_written_count/deviation;
			vData.reserve( count  );
		}
		m_record_ofs.seekg(0);
		DataStruct data;
		m_record_ofs.seekg( startpos * sizeof(DataStruct), std::ios_base::cur );
		deviation--;                    ////reason: because the file pointer will move ahead after we read data
		if( deviation ){
			deviation *=sizeof(DataStruct);
			unsigned int i = 0;
			while (m_record_ofs.peek() != EOF && i < count ) {
				m_record_ofs.read( (char *)&data, sizeof(DataStruct));
				vData.push_back(data);
				m_record_ofs.seekg( deviation, std::ios_base::cur );
				i++;
			}
		}else{
			unsigned int i = 0;
			while (m_record_ofs.peek() != EOF && i < count) {
				m_record_ofs.read( (char *)&data, sizeof(DataStruct));
				vData.push_back(data);
				i++;
			}
		}
	//    std::cout<<"=====read data=== Key:"<<data.instrument_id<<std::endl;
		m_record_ofs.clear();
	}
};











/*
Helper must provide three method:
RtnFieldHeader
WriteFieldToFile
ReadFieldFromFile
and one public const variable
support_txt2bin
*/

template<typename DataStruct, class CHelper>
class BinaryLoggerOffline{
public:
	////offline
    BinaryLoggerOffline(const std::string & name,const bool record_last, 
		CHelper helper,	const std::string & fmt = "csv", const std::string & date = ""
		);

	void txt2bin();
	void bin2txt();

public:
    const std::string m_logger_name;
private:
	const bool m_record_last;
	CHelper m_helper;

	boost::gregorian::date m_date;

	std::string m_record_filename_txt;
	std::string m_last_filename_txt;

	std::string m_record_filename_bin;
	std::string m_last_filename_bin;

	std::fstream m_record_ofs;
	std::fstream m_last_ofs;
};




////offline
template<typename DataStruct, class CHelper>
BinaryLoggerOffline<DataStruct, CHelper>::BinaryLoggerOffline(const std::string & name,const bool record_last,
									   CHelper helper,   const std::string & fmt,   const std::string & date 
									   )
	:m_logger_name(name), m_record_last(record_last),m_helper(helper)
{
	if( date.size() >=8 )
	{
		m_date = boost::gregorian::date_from_iso_string( date );
	}else{
		m_date = boost::gregorian::day_clock::local_day();
	}

	std::string t = boost::gregorian::to_iso_string( m_date );

	BinaryLoggerUtil::m_logger_filename_format.clear();
	BinaryLoggerUtil::m_logger_filename_format%name%"Record"%t%fmt;
	m_record_filename_txt = BinaryLoggerUtil::m_logger_filename_format.str();
	BinaryLoggerUtil::m_logger_filename_format.clear();
	BinaryLoggerUtil::m_logger_filename_format%name%"Record"%t%"bin";
	m_record_filename_bin = BinaryLoggerUtil::m_logger_filename_format.str();

	boost::filesystem::path cur_path( boost::filesystem::current_path() );
	std::cout<<"DataCenter Files Directory:"<<cur_path<<std::endl;

	cur_path /= m_record_filename_bin;
	m_record_filename_bin = cur_path.string();

	cur_path = boost::filesystem::current_path();
	cur_path /= m_record_filename_txt;
	m_record_filename_txt = cur_path.string();

//	if( boost::mpl::bool_<m_record_last>::value )
	if( m_record_last )
	{
		BinaryLoggerUtil::m_logger_filename_format.clear();
		BinaryLoggerUtil::m_logger_filename_format%name%"Last"%t%fmt;
		m_last_filename_txt = BinaryLoggerUtil::m_logger_filename_format.str();
		BinaryLoggerUtil::m_logger_filename_format.clear();
		BinaryLoggerUtil::m_logger_filename_format%name%"Last"%t%"bin";
		m_last_filename_bin = BinaryLoggerUtil::m_logger_filename_format.str();

		cur_path = boost::filesystem::current_path() ;
		cur_path /= m_last_filename_bin;
		m_last_filename_bin = cur_path.string();

		cur_path = boost::filesystem::current_path();
		cur_path /= m_last_filename_txt;
		m_last_filename_txt = cur_path.string();
	}
};




template<typename DataStruct, class CHelper>
void BinaryLoggerOffline<DataStruct, CHelper>::txt2bin(){
	if( !m_helper.support_txt2bin ) return;
	if( boost::filesystem::exists( m_record_filename_txt.c_str() ) )
	{
		std::ifstream ofs( m_record_filename_txt.c_str(),  std::ios::in  );
		std::ofstream m_record_ofs( m_record_filename_bin.c_str(),  std::ios::out |std::ios::binary | std::ios::trunc );

		if( ofs.fail() || m_record_ofs.fail() )
		{
			std::cout<<"===> Can not open file:"<<m_record_filename_txt<<" or "<<m_record_filename_bin<<" <=============="<<std::endl;
			ofs.clear();	m_record_ofs.clear();
			return;
		}
		std::cout<<"===> Translating Txt File:"<<m_record_filename_txt<<std::endl;
	
		std::string line;
		std::getline( ofs, line);////discard header
		DataStruct data;
		while (std::getline(ofs, line))
		{
			std::istringstream iss(line);
			m_helper.ReadFieldFromFile(data, iss); ///////read data from file
			m_record_ofs.write((const char *)&(data), sizeof(DataStruct) );
		}
		ofs.close(); m_record_ofs.close();
	}else
	{
		std::cout<<"===> The file:"<<m_record_filename_txt<<" does not exists!"<<std::endl;
	}

	if( m_record_last )
	{
		if( boost::filesystem::exists( m_last_filename_txt.c_str() ) )
		{
			std::ifstream ofs( m_last_filename_txt.c_str(),  std::ios::in  );
			std::ofstream m_last_ofs( m_last_filename_bin.c_str(),  std::ios::out |std::ios::binary | std::ios::trunc );

			if( ofs.fail() || m_last_ofs.fail() )
			{
				std::cout<<"===> Can not open file:"<<m_last_filename_txt<<" or "<<m_last_filename_bin<<" <=============="<<std::endl;
				ofs.clear(); m_last_ofs.clear();
				return;
			}
			std::cout<<"===> Translating Txt File:"<<m_last_filename_txt<<std::endl;
			std::string line;
			std::getline( ofs, line);////discard header
			DataStruct data;
			while (std::getline(ofs, line))
			{
				std::istringstream iss(line);
				m_helper.ReadFieldFromFile(data, iss); ///////read data from file
				m_last_ofs.write((const char *)&(data), sizeof(DataStruct) );
			}
			ofs.close(); m_last_ofs.close();
		}else{
			std::cout<<"===> The file:"<<m_last_filename_txt<<" does not exists!"<<std::endl;
		}
	}
}

template<typename DataStruct, class CHelper>
void BinaryLoggerOffline<DataStruct, CHelper>::bin2txt(){
	if( boost::filesystem::exists( m_record_filename_bin.c_str() ) )
	{
		std::ifstream m_record_ofs( m_record_filename_bin.c_str(), std::ios::in  | std::ios::binary );
		std::ofstream ofs( m_record_filename_txt.c_str(), std::ios::out | std::ios::trunc);
				
		if( ofs.fail() || m_record_ofs.fail() )
		{
			std::cout<<"===> Can not open file:"<<m_record_filename_txt<<" or "<<m_record_filename_bin<<" <=============="<<std::endl;
			ofs.clear();
			return;
		}
		std::cout<<"===> Translating Bin File:"<<m_record_filename_bin<<std::endl;
        
		ofs<<m_helper.RtnFieldHeader()<<std::endl; ////write header
				
		m_record_ofs.seekg(0);
		DataStruct data;
		while (m_record_ofs.peek() != EOF ) {
			m_record_ofs.read( (char *)&data, sizeof(DataStruct));
			m_helper.WriteFieldToFile(data, ofs);
		}

		ofs.close(); m_record_ofs.close();
	}else{
		std::cout<<"===> The file:"<<m_record_filename_bin<<" does not exists!"<<std::endl;
	}

	if( m_record_last )
	{
		if( boost::filesystem::exists( m_last_filename_bin.c_str() ) )
		{
			std::ifstream m_last_ofs( m_last_filename_bin.c_str(), std::ios::in  | std::ios::binary );
			std::ofstream ofs( m_last_filename_txt.c_str(),  std::ios::out | std::ios::trunc );
			if( ofs.fail() || m_last_ofs.fail() )
			{
				std::cout<<"===> Can not open file:"<<m_last_filename_txt<<" or "<<m_last_filename_bin<<" <=============="<<std::endl;
				ofs.clear();m_last_ofs.clear();
				return;
			}
			std::cout<<"===> Translating Bin File:"<<m_last_filename_bin<<std::endl;
			ofs<<m_helper.RtnFieldHeader()<<std::endl;   ////write header
			DataStruct data;
			while (m_last_ofs.peek() != EOF ) {
				m_last_ofs.read( (char *)&data, sizeof(DataStruct));
				m_helper.WriteFieldToFile(data, ofs);
			}
			ofs.close(); m_last_ofs.close();
		}else{
			std::cout<<"===> The file:"<<m_last_filename_bin<<" does not exists!"<<std::endl;
		}
	}	
}




#endif