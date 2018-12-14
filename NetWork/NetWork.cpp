// NetWork.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
// 
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "IPHLPAPI.lib")
#include <iphlpapi.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <set>
using namespace std;
//保存一个网络连接信息
struct NetWorkConection
{
	int index{};			//该连接在MIB_IFTABLE中的索引
	string description;		//网络描述（获取自GetAdapterInfo）
	string description_2;	//网络描述（获取自GetIfTable）
	unsigned int in_bytes;	//初始时已接收字节数
	unsigned int out_bytes;	//初始时已发送字节数
	wstring ip_address;	//IP地址
	wstring subnet_mask;	//子网掩码
	wstring default_gateway;	//默认网关
};
vector<NetWorkConection> adapters;
set<string> des_set;
vector<int> index;

wstring StrToUnicode(const char* str, bool utf8=false)
{
	wstring result;
	int size;
	size = MultiByteToWideChar((utf8 ? CP_UTF8 : CP_ACP), 0, str, -1, NULL, 0);
	if (size <= 0) return wstring();
	wchar_t* str_unicode = new wchar_t[size + 1];
	MultiByteToWideChar((utf8 ? CP_UTF8 : CP_ACP), 0, str, -1, str_unicode, size);
	result.assign(str_unicode);
	delete[] str_unicode;
	return result;
}

void GetAdapterInfo()
{
	PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
	unsigned long stSize = sizeof(IP_ADAPTER_INFO);
	int ret = GetAdaptersInfo(pIpAdapterInfo, &stSize);
	if (ERROR_BUFFER_OVERFLOW == ret)
	{
		delete pIpAdapterInfo;
		pIpAdapterInfo = (PIP_ADAPTER_INFO) new BYTE[stSize];
		ret = GetAdaptersInfo(pIpAdapterInfo, &stSize);
	}
	PIP_ADAPTER_INFO pIpAdapterInfoHead = pIpAdapterInfo;
	if (ret == ERROR_SUCCESS)
	{
		while (pIpAdapterInfo)
		{
			des_set.insert(pIpAdapterInfo->Description);
			NetWorkConection connection;
			connection.description = pIpAdapterInfo->Description;
			connection.ip_address = StrToUnicode(pIpAdapterInfo->IpAddressList.IpAddress.String);
			connection.subnet_mask = StrToUnicode(pIpAdapterInfo->IpAddressList.IpMask.String);
			connection.default_gateway = StrToUnicode(pIpAdapterInfo->GatewayList.IpAddress.String);
			adapters.push_back(connection);
			pIpAdapterInfo = pIpAdapterInfo->Next;
		}
	}
	if (pIpAdapterInfoHead)
	{
		delete pIpAdapterInfoHead;
	}
}
PMIB_IFTABLE m_pTable = nullptr;

void AutoSelected(MIB_IFTABLE* lf_table)
{
	int connect_selected = 0;
	unsigned int max_in_out_bytes{};
	unsigned int in_out_bytes;
	for (auto i = 0; i < index.size(); ++i)
	{
		if (lf_table->table[index.at(i)].dwOperStatus == IF_OPER_STATUS_OPERATIONAL)
		{
			in_out_bytes = lf_table->table[index.at(i)].dwInOctets + lf_table->table[index.at(i)].dwOutOctets;
			if (in_out_bytes > max_in_out_bytes)
			{
				max_in_out_bytes = in_out_bytes;
				connect_selected = i;
			}
		}
	}
}
void NetSpeed()
{

	DWORD m_dwAdapters = 0;
	unsigned long uRetCode = GetIfTable(m_pTable, &m_dwAdapters, false);
	if (uRetCode == ERROR_NOT_SUPPORTED)
	{
		return;
	}
	if (uRetCode == ERROR_INSUFFICIENT_BUFFER)
	{
		if (m_pTable == nullptr)
		{
			m_pTable = (PMIB_IFTABLE)new BYTE[m_dwAdapters];
		}
	}
	DWORD dwLastIn = 0;
	DWORD dwLastOut = 0;
	DWORD dwBandIn = 0;
	DWORD dwBandOut = 0;
	while (true)
	{
		if (GetIfTable(m_pTable, &m_dwAdapters, false) == ERROR_SUCCESS)
		{
			index.clear();
			DWORD dwInOctets = 0;
			DWORD   dwOutOctets = 0;
			for (int i = 0; i < m_pTable->dwNumEntries; ++i)
			{
				MIB_IFROW   Row = m_pTable->table[i];
				string dx = (const char*)Row.bDescr;
		
				if (des_set.find(dx) != des_set.end())
				{
					index.push_back(i);
				//	cout << dx << endl;
				}
				else
				{
					continue;
				}
//				if (Row.dwType == IF_TYPE_ETHERNET_CSMACD || Row.dwType == IF_TYPE_IEEE80211)
				{
				//	cout <<"IF_TYPW: "<< Row.dwType << endl;
					dwInOctets += Row.dwInOctets;
					dwOutOctets += Row.dwOutOctets;
				}
			}
			AutoSelected(m_pTable);
			dwBandIn = dwInOctets - dwLastIn;       //下载速度
			dwBandOut = dwOutOctets - dwLastOut;    //上传速速
			if (dwLastIn <= 0)
			{
				dwBandIn = 0;
			}
			else
			{
				dwBandIn = dwBandIn / 1024; //b转换成kb
			}

			if (dwLastOut <= 0)
			{
				dwBandOut = 0;
			}
			else
			{
				dwBandOut = dwBandOut / 1024;   //b转换成kb
			}

			dwLastIn = dwInOctets;
			dwLastOut = dwOutOctets;
			std::wcout << m_pTable->table->wszName << "\n";
			std::cout << "收到字节:" << dwLastIn << "bytes\n";
			std::cout << "发送字节:" << dwLastOut << "bytes\n";
			std::cout << "下行速度:" << dwBandIn << "KB\n";
			std::cout << "上行速度: " << dwBandOut << "KB\n";
			std::cout << "--------------------------\n";
		}
		
	   std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void main()
{
	GetAdapterInfo();
	NetSpeed();
}