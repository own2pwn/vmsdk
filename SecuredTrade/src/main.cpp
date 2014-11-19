/*! \mainpage Developer documentation
 *
 * \section 买家购买交易包
 *描述：买卖 双方商量好交易价格，超时时间，裁决人等，买家组合好交易包\n
 *函数：bool ProcessContract1(const FIRST_CONTRACT* pContract);\n


 *\section 卖家确认包
 *描述：卖家接受买家的转账\n
 *函数：bool ProcessContract2(const NEXT_CONTRACT* pContract);\n
 *
 *
 *\section 买家申诉包
 *描述：卖家不发货，买家提出申诉\n
 *函数：bool ProcessContract3(const NEXT_CONTRACT* pContract);\n
 *
 *
 *\section 仲裁结果包
 *描述：仲裁判决结果\n
 *函数：bool ProcessContract3(const NEXT_CONTRACT* pContract);\n
 *
 *.
 */

#include <string.h>
#include<stdlib.h>
#include"VmSdk.h"

#define ACCOUNT_ID_SIZE 6
#define MAX_ARBITRATOR 3
#define MAX_ACCOUNT_LEN 20
#define HASH_SIZE	32


typedef struct  {
	unsigned char nType;					//!<类型
	unsigned char nArbitratorCount;			//!<仲裁者个数
	ACCOUNT_ID 	buyer;						//!<买家ID（采用6字节的账户ID）
	ACCOUNT_ID seller;						//!<卖家ID（采用6字节的账户ID）
	ACCOUNT_ID arbitrator[MAX_ARBITRATOR];	//!<仲裁者ID（采用6字节的账户ID）
	int nHeight;							//!<超时绝对高度
	Int64 nFineMoney;						//!<卖家违约后最大罚款金额
	Int64 nPayMoney;						//!<买家向卖家支付的金额
	Int64 nFee;								//!<仲裁手续费
	Int64 ndeposit;							//!<仲裁押金,申诉时从仲裁者扣除的押金(如果仲裁不响应钱归买家否则买家退还给仲裁者)
} FIRST_CONTRACT;

typedef struct {
	unsigned char nType;				//!<交易类型
	unsigned char hash[HASH_SIZE];		//!<上一个交易包的哈希
} NEXT_CONTRACT;

typedef struct {
	unsigned char nType;				//!<交易类型
	unsigned char hash[HASH_SIZE];		//!<上一个交易的哈希
	Int64 nMinus;
}ARBIT_RES_CONTRACT;

enum Role {
	BUYER = 1,
	SELLER,
	ARBITRATOR,
	UNDIFUNE_ROLE
};
/**
 * @brief   step1:block高度有效性\n
 * 			step2:买家和卖家地址是否有效\n
 *    		step3:仲裁者地址是否存在，仲裁者个数是否合法，仲裁者是否关联脚本\n
 * 			step4:卖家的自由金额是否足够赔偿\n
 * 			step5:权限自定义字段中的值与各角色是否匹配
 */

bool CheckContract1(const FIRST_CONTRACT* pContract) {
	//step1
	unsigned long nRunTimeHeight = GetCurRunEnvHeight();
	if ((unsigned long)pContract->nHeight>nRunTimeHeight)
		return false;

	//step2
	if (!IsAccountRegID((const void* const)pContract->buyer.accounid) ||
			! IsAccountRegID((const void* const)pContract->seller.accounid))
		return false;

	//step3
	if (1 != pContract->nArbitratorCount || IsAccountRegID((const void*)pContract->arbitrator[0].accounid) )
		return false;

	//step4
	Int64	nTotalFreeMoney;
	if (!QueryAccountBalance((const unsigned char* const)pContract->seller.accounid,ACOUNT_ID,&nTotalFreeMoney))
		return false;
	COMP_RET ret = Int64Compare(&pContract->nFineMoney,&nTotalFreeMoney);
	if (COMP_ERR == ret || COMP_LARGER == ret)
		return false;

	//step5
	char szRole[10] = {0};
	if (GetAuthoritedDefine((const void* const ) pContract->buyer.accounid, szRole, sizeof(szRole) / sizeof(szRole[0]))
			|| BUYER != szRole[0])
		return false;
	if (GetAuthoritedDefine((const void* const ) pContract->seller.accounid, szRole, sizeof(szRole) / sizeof(szRole[0]))
			|| SELLER != szRole[0])
		return false;
	if (GetAuthoritedDefine((const void* const ) pContract->arbitrator[0].accounid, szRole, sizeof(szRole) / sizeof(szRole[0]))
			|| ARBITRATOR != szRole[0])
		return false;

	return true;
}

/**
 * @brief 	step1:hash的有效性\n
 * 			step2:从数据库中读取数据判断上一个交易是否已经处理（是否已经有其他卖家确认包，防止卖家重复确认），如果已经处理则此交易无效返回。\n
 * 			step3:交易签名账户地址是否是第一个交易中卖家的地址\n
 * @param pContract
 */

bool CheckContract2(const NEXT_CONTRACT* pContract,FIRST_CONTRACT* pFirstContract) {
	//step1
	if (!GetTxContacts((const unsigned char * const)pContract->hash,(void* const)pFirstContract,sizeof(FIRST_CONTRACT)))
		return false;

	//step2
	char szValue[2] = {0};
	if (!ReadDataValueDB(pContract->hash,sizeof(pContract->hash),szValue,sizeof(szValue)/sizeof(szValue[0])))
		return false;

	//step3
	ACCOUNT_ID sellID;
	if (ACCOUNT_ID_SIZE != GetAccounts((const unsigned char *)pContract->hash,(void*)sellID.accounid,sizeof(sellID))
			||!memcmp((const void*)sellID.accounid,(const void*)pFirstContract->seller.accounid,ACCOUNT_ID_SIZE) )
		return false;
	return true;
}

/**
 * @brief	step1：hash的有效性\n
 * 			step2：上一个交易是否有处理过
 * @param pContract
 */
bool CheckContract3(const NEXT_CONTRACT* pContract, FIRST_CONTRACT* pFirstContract) {
	//step1
	NEXT_CONTRACT contract2;
	if (!GetTxContacts((const unsigned char * const ) pContract->hash, (void* const ) &contract2, sizeof(NEXT_CONTRACT)))
		return false;

	if (!GetTxContacts((const unsigned char * const ) contract2.hash, (void* const ) pFirstContract,
			sizeof(FIRST_CONTRACT)))
		return false;

	//step2
	char szValue[2] = { 0 };
	if (!ReadDataValueDB(pContract->hash, sizeof(pContract->hash), szValue, sizeof(szValue) / sizeof(szValue[0])))
		return false;

	return true;
}

/**
 * @brief	step1：检查第三个包的hash有效性\n
 * 			step2；扣钱金额的有效性\n
 * 			step3：检测上一个交易是否有处理
 * @param pContract
 */
bool CheckContract4(const ARBIT_RES_CONTRACT* pContract, FIRST_CONTRACT* pFirstContract) {
	//step1
	NEXT_CONTRACT contract3;
	if (!GetTxContacts((const unsigned char * const ) pContract->hash, (void* const ) &contract3, sizeof(contract3)))
		return false;

	NEXT_CONTRACT contract2;
	if (!GetTxContacts((const unsigned char * const ) pContract->hash, (void* const ) &contract2, sizeof(NEXT_CONTRACT)))
		return false;

	if (!GetTxContacts((const unsigned char * const ) contract2.hash, (void* const ) pFirstContract,
			sizeof(FIRST_CONTRACT)))
		return false;

	//step 2
	COMP_RET ret = Int64Compare(&pContract->nMinus, &pFirstContract->nFineMoney);
	if (COMP_ERR == ret || COMP_LARGER == ret)
		return false;

	//step3
	char szValue[2] = { 0 };
	if (!ReadDataValueDB(pContract->hash, sizeof(pContract->hash), szValue, sizeof(szValue) / sizeof(szValue[0])))
		return false;
	return true;
}
/**
 * @brief step1:从买家的自由金额中扣除金额(应支付给卖家的钱)\n
 *        step2:把上一步操作的钱转入买家的冻结金额中
 * @param vmoper 操作账户的结构体数组
 * @param nVmSize 数组大小
 * @param pContract 第一个合约结构
 */
bool OperAccount1(VM_OPERATE *vmoper,unsigned char nVmSize,const void* pContract) {
	if (NULL == vmoper || NULL == pContract || nVmSize!=2)
		return false;

	FIRST_CONTRACT* pContractData = (FIRST_CONTRACT*)pContract;
	vmoper[0].TYPE = ACOUNT_ID;
	vmoper[0].accountid = pContractData->buyer;
	vmoper[0].opeatortype = MINUS_FREE;
	vmoper[0].outheight = pContractData->nHeight;
	vmoper[0].money = pContractData->nPayMoney;

	vmoper[1].TYPE = ACOUNT_ID;
	vmoper[1].accountid = pContractData->buyer;
	vmoper[1].opeatortype = ADD_FREEZD;
	vmoper[1].outheight = pContractData->nHeight;
	vmoper[1].money = pContractData->nPayMoney;

  return true;
}

/**
 * @brief step1:从买家的冻结转入卖家的冻结金额中\n
 * 		  step2:卖家自由金额中扣除最大罚款到冻结金额中
 * @param vmoper 操作账户的结构体数组
 * @param nVmSize 数组大小
 * @param pContract 第二个合约结构
 */
bool OperAccount2(VM_OPERATE *vmoper, unsigned short nVmSize,FIRST_CONTRACT* pFirstContract) {
	if (NULL == vmoper || NULL == pFirstContract || nVmSize != 4)
		return false;

	//step1
	vmoper[0].TYPE = ACOUNT_ID;
	vmoper[0].accountid = pFirstContract->buyer;
	vmoper[0].opeatortype = MINUS_FREEZD;
	vmoper[0].outheight = pFirstContract->nHeight;
	vmoper[0].money = pFirstContract->nPayMoney;

	vmoper[1].TYPE = ACOUNT_ID;
	vmoper[1].accountid = pFirstContract->seller;
	vmoper[1].opeatortype = ADD_FREEZD;
	vmoper[1].outheight = pFirstContract->nHeight;
	vmoper[1].money = pFirstContract->nPayMoney;

	//step 2
	vmoper[2].TYPE = ACOUNT_ID;
	vmoper[2].accountid = pFirstContract->buyer;
	vmoper[2].opeatortype = MINUS_FREE;
	vmoper[2].outheight = pFirstContract->nHeight;
	vmoper[2].money = pFirstContract->nFineMoney;

	vmoper[3].TYPE = ACOUNT_ID;
	vmoper[3].accountid = pFirstContract->seller;
	vmoper[3].opeatortype = ADD_FREEZD;
	vmoper[3].outheight = pFirstContract->nHeight;
	vmoper[3].money = pFirstContract->nFineMoney;
	return true;
}

/**
 * @brief 	从仲裁者自由金额中扣除指定费用到买家冻结金额中。
 * @param vmoper
 * @param nVmSize
 * @param pContract
 */
bool OperAccount3(VM_OPERATE *vmoper, unsigned short nVmSize,FIRST_CONTRACT* pFirstContract) {
	if (NULL == vmoper || NULL == pFirstContract || nVmSize != 2)
		return false;

	vmoper[0].TYPE = ACOUNT_ID;
	vmoper[0].accountid = pFirstContract->arbitrator[0];
	vmoper[0].opeatortype = MINUS_FREE;
	vmoper[0].outheight = pFirstContract->nHeight;
	vmoper[0].money = pFirstContract->ndeposit;

	vmoper[1].TYPE = ACOUNT_ID;
	vmoper[1].accountid = pFirstContract->buyer;
	vmoper[1].opeatortype = ADD_FREEZD;
	vmoper[1].outheight = pFirstContract->nHeight;
	vmoper[1].money = pFirstContract->ndeposit;
	return true;
}

/**
 * @brief 	1.	从卖家的冻结金额中扣除罚款到买家的自由金额
			2.	从买家的冻结金额中返还仲裁者扣给买家的钱
			3.	从买家自由金额中扣除仲裁手续费给仲裁者
 * @param vmoper
 * @param nVmSize
 * @param pContract
 */
bool OperAccount4(VM_OPERATE *vmoper, unsigned short nVmSize,FIRST_CONTRACT* pFirstContract) {
	if (NULL == vmoper || NULL == pFirstContract || nVmSize != 2)
		return false;

	//step 1
	vmoper[0].TYPE = ACOUNT_ID;
	vmoper[0].accountid = pFirstContract->seller;
	vmoper[0].opeatortype = MINUS_FREEZD;
	vmoper[0].outheight = pFirstContract->nHeight;
	vmoper[0].money = pFirstContract->nFineMoney;

	vmoper[1].TYPE = ACOUNT_ID;
	vmoper[1].accountid = pFirstContract->buyer;
	vmoper[1].opeatortype = ADD_FREE;
	vmoper[1].outheight = pFirstContract->nHeight;
	vmoper[1].money = pFirstContract->nFineMoney;

	//step 2
	vmoper[2].TYPE = ACOUNT_ID;
	vmoper[2].accountid = pFirstContract->buyer;
	vmoper[2].opeatortype = MINUS_FREEZD;
	vmoper[2].outheight = pFirstContract->nHeight;
	vmoper[2].money = pFirstContract->ndeposit;

	vmoper[3].TYPE = ACOUNT_ID;
	vmoper[3].accountid = pFirstContract->arbitrator[0];
	vmoper[3].opeatortype = ADD_FREE;
	vmoper[3].outheight = pFirstContract->nHeight;
	vmoper[3].money = pFirstContract->ndeposit;

	//step 3
	vmoper[4].TYPE = ACOUNT_ID;
	vmoper[4].accountid = pFirstContract->buyer;
	vmoper[4].opeatortype = MINUS_FREE;
	vmoper[4].outheight = pFirstContract->nHeight;
	vmoper[4].money = pFirstContract->nFee;

	vmoper[5].TYPE = ACOUNT_ID;
	vmoper[5].accountid = pFirstContract->arbitrator[0];
	vmoper[5].opeatortype = ADD_FREE;
	vmoper[5].outheight = pFirstContract->nHeight;
	vmoper[5].money = pFirstContract->nFee;
	return true;
}

/**
 * @brief 1.	从买家的自由金额转入买家的冻结中
 * @param data 操作账户的结构体指针
 * @param conter 操作次数
 * @return
 */
bool WriteOutput1( const VM_OPERATE* data, unsigned short conter){
	return WriteOutput(data,conter);
}

/**
 * @brief	1.	从买家的输出冻结转到卖家的冻结。\n
			2.	从卖家的自由金额中扣款转到卖家的冻结中
 * @param data 操作账户的结构体指针
 * @param conter 操作次数
 * @return
 */
bool WriteOutput2( const VM_OPERATE* data, unsigned short conter){
	return WriteOutput(data,conter);
}

/**
 * @brief	1.	从仲裁者自由金额中扣除少量费用到买家冻结金额中。
 * @param data 操作账户的结构体指针
 * @param conter 操作次数
 * @return
 */
bool WriteOutput3( const VM_OPERATE* data, unsigned short conter){
	return WriteOutput(data,conter);
}

/**
 * @brief	1.	从仲裁者的冻结恢复一定金额到自由金额中\n
			2.	从卖家的冻结金额中扣除费用给买家\n
			3.	从卖家的自由金额中扣除仲裁费用给仲裁者
 * @param data 操作账户的结构体指针
 * @param conter 操作次数
 * @return
 */
bool WriteOutput4( const VM_OPERATE* data, unsigned short conter){
	return WriteOutput(data,conter);
}

/**
 * @brief	将第一个交易的哈希作为key写入数据库
 * @return
 */
bool WriteDB2(const char* pKey,FIRST_CONTRACT* pFirstContract) {
	bool bFlag = true;
	return WriteDataDB((const void* const)pKey,32,&bFlag,1,pFirstContract->nHeight);
}

/**
 * @brief	将第二个交易的哈希作为key写入数据库
 * @return
 */
bool WriteDB3(const char* pKey,FIRST_CONTRACT* pFirstContract) {
	bool bFlag = true;
	return WriteDataDB((const void* const)pKey,32,&bFlag,1,pFirstContract->nHeight);
}

/**
 * @brief	将第三个交易的哈希作为key写入数据库
 * @return
 */
bool WriteDB4(const char* pKey,FIRST_CONTRACT* pFirstContract) {
	bool bFlag = true;
	return WriteDataDB((const void* const)pKey,32,&bFlag,1,pFirstContract->nHeight);
}
/**
 * @brief 处理第一个交易
 * @param pContract
 * @return
 */
bool ProcessContract1(const FIRST_CONTRACT* pContract) {
	if (!CheckContract1(pContract)) {
		VM_OPERATE vmOper[2];
		if (OperAccount1(vmOper, sizeof(vmOper) / sizeof(vmOper[0]), pContract))
			return WriteOutput1(vmOper, sizeof(vmOper)/sizeof(vmOper[0]));
	}
	return false;
}

/**
 * @brief 卖家确认
 * @param pContract 合约内容
 * @return
 */
bool ProcessContract2(const NEXT_CONTRACT* pContract) {
	VM_OPERATE vmOper[4];
	FIRST_CONTRACT firstcontract;
	if (!CheckContract2(pContract,&firstcontract)) {
		if (OperAccount2(vmOper, sizeof(vmOper) / sizeof(vmOper[0]), &firstcontract)) {
			if (WriteDB2((const char*)pContract->hash,&firstcontract))
				return WriteOutput2(vmOper, sizeof(vmOper)/sizeof(vmOper[0]));
		}
	}
	return false;
}

/**
 * @brief 执行申诉
 * @param pContract 合约内容
 * @return
 */
bool ProcessContract3(const NEXT_CONTRACT* pContract) {
	VM_OPERATE vmOper[2];
	FIRST_CONTRACT firstcontract;
	if (CheckContract3(pContract,&firstcontract)) {
		if (OperAccount3(vmOper, sizeof(vmOper) / sizeof(vmOper[0]), &firstcontract)) {
			if (WriteDB3((const char*)pContract->hash,&firstcontract))
				return WriteOutput3(vmOper, sizeof(vmOper)/sizeof(vmOper[0]));
		}
	}
	return false;
}

/**
 * @brief 执行仲裁结果
 * @param pContract 仲裁结果合约结构体
 * @return
 */
bool ProcessContract4(const ARBIT_RES_CONTRACT* pContract) {
	VM_OPERATE vmOper[6];
	FIRST_CONTRACT firstcontract;
	if (CheckContract4(pContract,&firstcontract)) {
		if (OperAccount4(vmOper, sizeof(vmOper) / sizeof(vmOper[0]), &firstcontract)) {
			if (WriteDB4((const char*)pContract->hash,&firstcontract))
				return WriteOutput4(vmOper, sizeof(vmOper)/sizeof(vmOper[0]));
		}
	}
	return false;
}

int main() {
	__xdata static  char pContract[512];
	unsigned long len = 512;
	GetMemeroyData(pContract,len);

	#pragma data_alignment = 1
//	FIRST_CONTRACT firstcontract;
//	memcpy(firstcontract.buyer.accounid,"111111",ACCOUNT_ID_SIZE);
//	memcpy(firstcontract.seller.accounid,"222222",ACCOUNT_ID_SIZE);
//	memcpy(firstcontract.arbitrator[0].accounid,"333333",ACCOUNT_ID_SIZE);
//	firstcontract.nType = 1;
//	Int64Inital(&firstcontract.nFineMoney,"\x64",1);//100
//	Int64Inital(&firstcontract.nPayMoney,"\x50",1);//80
//	Int64Inital(&firstcontract.ndeposit,"\x14",1);//20
//	Int64Inital(&firstcontract.nFee,"\x64",1);//10
//	memcpy(pContract,&firstcontract,sizeof(FIRST_CONTRACT));

//	NEXT_CONTRACT secondcontract;
//	memcpy(secondcontract.hash,"11111111222222223333333344444444",32);
//	secondcontract.nType = 2;
//	memcpy(pContract,&secondcontract,sizeof(NEXT_CONTRACT));
	switch (pContract[0]) {
	case 1:
		ProcessContract1((FIRST_CONTRACT*)pContract);
		break;

	case 2:
		ProcessContract2((NEXT_CONTRACT*)pContract);
		break;

	case 3:
		ProcessContract3((NEXT_CONTRACT*)pContract);
		break;

	case 4:
		ProcessContract4((ARBIT_RES_CONTRACT*)pContract);
		break;
	}
}










