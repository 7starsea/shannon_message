///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
#ifndef DATA_STRUCT_HEADER_Quoter
#define DATA_STRUCT_HEADER_Quoter


//修改选项
typedef struct TestModifyInfoStruct{
    //合约名称
    char option_id[31];
    //修改选项
    int	option;
    //修改值
    double option_value;
	const char * key_id() const{
		return this->option_id;
	}	
}TestModifyInfoStruct;

typedef struct TestSTructFirst
{
	char id[31];
	char update_time[15];
	double value1;
	double value2;
	double value3;
	double value4;
	const char * key_id() const{
		return this->id;
	}
}TestSTructFirst;
typedef struct TestSTructSecond
{
	char id[31];
	char update_time[15];
	double value1;
	double value2;
	double value3;
	double value4;
	const char * key_id() const{
		return this->id;
	}
}TestSTructSecond;





#endif