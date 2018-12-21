///
/// Copyright(c) 2018 Aimin Huang
/// Distributed under the MIT License (http://opensource.org/licenses/MIT)
///
//Configuration:GeneratorOptionConfig,QuoterOptionConfig|GeneratorSystemConfig,QuoterSystemConfig
#ifndef MonitorDataStruct_Header_Monitor
#define MonitorDataStruct_Header_Monitor


#include <string>
#include <map>


//竞价参数
typedef struct AuctionParaStruct{
	//开盘竞价响应延迟
	int open_auction_insert_delay;
	//熔断竞价响应延迟
	int fuse_auction_insert_delay;
	//收盘竞价响应延迟
	int close_auction_insert_delay;
	//竞价最小下单手数
	int auction_quote_vol;
	//竞价检查的间隔
	int auction_gap_sec;
}AuctionParaStruct;

//价格过滤器参数
typedef struct PriceFilterParamsStruct{
	//是否为好行情(在无效市场中)
	double max_recovery_price_jump;
	//是否为好行情(在有效市场中)
	double max_acceptable_price_jump;
	//有效市场的阈值(连续多个好行情，确认为有效市场)
	int recovery_successive_good_price_count;
	//行情修正参数(重要)
	double md_revised_important;
	//行情修正参数(普通)
	double md_revised_common;
	//行情修正参数(不重要)
	double md_revised_unimportant;
}PriceFilterParamsStruct;

typedef struct EmaParamsStruct{
	//ema_lamada参数(用于计算成交量的ema算法)
	double lambda;//ema_lamada;
	//sensitive_spread的参数ks
	double sensitive_spread_ks;//sensitive_spread_ks;
	//sensitive_spread的参数ms
	double sensitive_spread_ms;//sensitive_spread_ms;
}EmaParamsStruct;
typedef struct ContractNameInfo{
	//标的物现货代码
	char id[31];
	//标的合约类型
	short future_type;
	//上一个主力(如果是非主力合约)
	char pre_main_future[31];
	//下一个主力(如果是非主力合约)
	char post_main_future[31];	
}ContractNameInfo;
//期权的统一配置信息
typedef struct QuoterSystemConfig{
	//期权订阅前缀
	//char option_subscribe_prefix[31];
	//启动开仓的上限(资金)
	double capital_can_open;
	//启动平仓的下限(资金)
	double capital_only_close;
	//非线性保证金系数
	double margin_non_linear_coff;
	//风险厌恶系数
	double risk_aversion;
	//保证金厌恶系数
	double margin_aversion;
	//期货价格每分钟的波动率
	double future_volatility;
	//费用比例（手续费+价差）
	double fee;
	//对冲成本系数
	double hedge_cost;
	//交易员规定的最小下单量
	int quote_min_vol_shannon;
	//交易所规定的最小下单量
	int quote_min_vol_exchange;
	//每一轮的处理数量
	int capacity_to_exchange;
	//有效时间
	int pricing_valid_time;
	//标的期货信息
	ContractNameInfo future;
	//过滤参数
	PriceFilterParamsStruct filter_para;//price_filter_para;
	//竞价参数
	AuctionParaStruct  auction_para;
	//ema_lamada参数
	EmaParamsStruct ema_params;
	//控制总delta的非线性系数
	double delta_non_linear;

	QuoterSystemConfig(){
		memset( this, 0 ,sizeof(QuoterSystemConfig) );
	}
	const char * key_id()const{
		return this->future.id;
	}
	void reset_key_id(const char * key){
		strncpy(this->future.id, key, sizeof(this->future.id)-1);
	}

}QuoterSystemConfig;

//期权配置信息
typedef struct QuoterOptionConfig{
	//名字
	char option_id[31];    /////currently used
	//预计收益
	double profit;
	//预期持仓时间（分钟）
	double position_time;
	//交易成本
	double traded_cost;
	//大容忍
	double tolerance_greater;
	//小容忍
	double tolerance_less;
	//状态容忍
	double tolerance_status;
	//敏感容忍
	double tolerance_sensitive;
	//标的波动幅度
	double underlying_fluct_range;
	//可用资金
	double option_asset;
	//原始价格的权重
	double smooth_price_weight;
	//calibrated价格权重
	double calibrated_price_weight;
	//parity价格权重
	double parity_price_weight;
	//基础波动率的权重
	double fundamental_price_weight;
	//波动率的波动幅度
	double volatility_fluct_range;
	//波动率的误差系数
	double volatility_error_weight;
	//borrow rate 误差系数
	double borrow_rate_error_weight;
	//是否允许开仓(因为快到期不想持仓，所以只平不开)
	bool can_open;
	//是否允许下单
	bool can_quote;
	//是否允许应价
	bool can_for_quote;

	QuoterOptionConfig(){
		memset( this, 0 ,sizeof(QuoterOptionConfig) );
	}
	const char * key_id() const{
		return this->option_id;
	}
	void reset_key_id(const char * key){
		strncpy(this->option_id, key, sizeof(this->option_id)-1);
	}
}QuoterOptionConfig;








//价格过滤器参数
struct GeneratorPriceFilterParams
{
	double max_recovery_price_jump;
	double max_acceptable_price_jump;
	int recovery_successive_good_price_count;
};

//行情修改器参数
struct GeneratorMdRevisingWeights
{
	double important_level_weight;
	double common_level_weight;
	double unimportant_level_weight;
};

struct GeneratorSystemConfig
{
	//基础产品ID
	char base_product_id[10];
	//交易所ID
	char exchange_id[8];
	//Regression组件的地址和端口号
	char regression_params_source[31];
	//发送期权定价参数使用的端口号
	int network_port;
	//波动率校准算法调用的时间间隔
	int calibration_interval;
	//最小变动价位
	double tick_size;
	//标的行情过滤器参数
	GeneratorPriceFilterParams price_filter_params;
	//标的行情修改器参数
	GeneratorMdRevisingWeights md_revising_weights;

	const char * key_id()const{
		return this->base_product_id;
	}
	void reset_key_id(const char * key){
		strncpy(this->base_product_id, key, sizeof(this->base_product_id)-1);
	}
};


//到期月份属性
struct GeneratorExpiryMonthConfig
{
	//该月份的KeyID
	char month_key_id[31];
	///ASSTR 基础产品ID
	char base_product[10];
	///ASSTR 该月份的ID
	char expiry_month_id[5];
	//该到期月类型
	int expiry_month_type;
	///ASSTR 之前活跃到期月ID，本身是活跃月份或之前没有活跃到期月为NULL
	char pre_active_month_id[5];
	///ASSTR 之后活跃到期月ID，本身是活跃月份或之后没有活跃到期月为NULL
	char post_active_month_id[5];
	//波动率来源类型
	int sigma_source_type;
	//无风险利率
	double riskfree_rate;
	//波动率校准算法需要的最少点数
	int min_effective_points_for_calibration;
	//波动率校准算法因子
	double special_calibration_factor;
	//初始的BorrowRate
	double init_borrow_rate;
	//BorrowRate的平滑系数
	int br_ema_length;
	//计算BorrowRate时使用的期权成交量调整参数
	int option_volume_adjustment;

	const char * key_id()const{
		return this->month_key_id;
	}
	void reset_key_id(const char * key){
		strncpy(this->month_key_id, key, sizeof(this->month_key_id)-1);
	}
};

//各BorrowRate的权重系数
struct GeneratorBorrowRateWeights
{
	//基础BorrowRate在合成BorrowRate中所占的权重
	double fundamental_weight;
	//当月BorrowRate在合成BorrowRate中所占的权重
	double monthly_weight;
	//自身隐含BorrowRate在合成BorrowRate中所占的权重
	double option_weight;
};

//过滤器参数
struct GeneratorFilterParams
{
	int window_size;
	double dif_threshold;
	double init_value;
};

//期权属性
//期权属性
struct GeneratorOptionConfig
{
	//期权ID
	char option_id[31];
	//波动率平滑系数
	int sigma_ema_length;
	//期权基础波动率
	double fundamental_sigma;
	//期权初始化波动率
	double init_benchmark_sigma;
	//期权初始化校准波动率
	double init_calibrated_sigma;
	//行情波动率过滤阈值
	double md_sigma_filter_dif_threshold;
	//平滑波动率过滤阈值
	double smoothed_sigma_filter_dif_threshold;
	//校准波动率过滤阈值
	double calibrated_sigma_filter_dif_threshold;
	//基础BorrowRate权重
	double fundamental_br_weight;
	//当月BorrowRate权重
	double monthly_br_weight;
	//期权自身BorrowRate权重
	double option_br_weight;
	//BorrowRate平滑使用的EMA算法的均线长度
	int br_ema_length;
	//期权基础BorrowRate
	double fundamental_borrow_rate;
	//期权初始化BorrowRate
	double init_borrow_rate;
	//BorrowRate过滤阈值
	double br_filter_dif_threshold;

	const char * key_id()const{
		return this->option_id;
	}
	void reset_key_id(const char * key){
		strncpy(this->option_id, key, sizeof(this->option_id)-1);
	}
};


#endif
