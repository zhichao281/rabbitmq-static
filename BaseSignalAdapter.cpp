
#include "BaseSignalAdapter.h"
#include "amqp_tcp_socket.h"
#include <windows.h>
#include <thread>



const std::string EXCHANGE_TYPE_DIRECT("direct");
const std::string EXCHANGE_TYPE_FANOUT("fanout");
const std::string EXCHANGE_TYPE_TOPIC("topic");

BaseSignalAdapter::BaseSignalAdapter()
{
	m_strExchangeName = "amq.topic";
	m_pSignalListenerThread.reset();


	m_pRabbitmq = nullptr;
}

BaseSignalAdapter::~BaseSignalAdapter()
{

	string errmsg;
	if (nullptr != m_pRabbitmq.get())
	{
		m_pRabbitmq->Disconnect(errmsg);
		m_pRabbitmq.reset();
	}
	m_pSignalListenerThread.reset();
}

int32_t BaseSignalAdapter::Connect(const std::string & host, int port,
	const std::string & username,
	const std::string & password,
	const std::string & vhost, string &ErrorReturn)
{


	m_pRabbitmq = std::make_shared<CRabbitMQ>();
	int res=m_pRabbitmq->Connect(host, port, username, password, vhost, ErrorReturn);
	if (res<0)
	{
		m_pRabbitmq.reset();
	}
	return res;
}

int BaseSignalAdapter::joinSignalChannel(std::string url, std::string userId, ThreadSignalListener mWSSignalListener)
{
	m_strQueue = userId;

	string ErrorReturn;
	if (m_pRabbitmq.get() != nullptr)
	{
		//����exchange
		m_pRabbitmq->exchangeDeclare(m_strExchangeName, EXCHANGE_TYPE_TOPIC, false, false, false, ErrorReturn);
		
		//ÿ���ͻ���ʹ��һ��˽�еĶ���
		m_pRabbitmq->queueDeclare(userId, false, false, false, false, ErrorReturn);

		std::string UniRoutingKey = getUniRoutingKey(url, userId);

		std::string BroadRoutingKey = getBroadRoutingKey(url);

		if (UniRoutingKey.size() > 0)
		{
			m_pRabbitmq->bindQueue(userId, m_strExchangeName, UniRoutingKey);
		}

		if (BroadRoutingKey.size() > 0)
		{
			m_pRabbitmq->bindQueue(userId, m_strExchangeName, BroadRoutingKey);
		}

		SetMessageReceived(mWSSignalListener);

		return 0;
	}
	else
	{
		return -1;
	}
}


int32_t BaseSignalAdapter::quitSignalChannel(std::string &ErrorReturn)
{
	if (m_pRabbitmq.get() != nullptr)
	{
		int res=m_pRabbitmq->Disconnect();
		return res;
	}
	return 0;
}

#include  <iostream>
#include <sstream>
using namespace  std;
void BaseSignalAdapter::sendMessage(std::string  channelUri, std::string  userId, std::string  message)
{
	if (m_pRabbitmq.get() != nullptr)
	{
		string errmsg;

	/*	std::string UniRoutingKey = getUniRoutingKey(channelUri, userId);
		UniRoutingKey = "wsrtc_vpclient_test_com.vpclient_test.send2.broad";
		m_pRabbitmq->exchangeDeclare(m_strExchangeName, EXCHANGE_TYPE_TOPIC, false, false, false, errmsg);
		m_pRabbitmq->basicPublish(UniRoutingKey,  message.c_str(), errmsg);
*/

		//����exchange
		CRabbitMQ Listener;
		Listener.Connect();
		Listener.exchangeDeclare(m_strExchangeName, EXCHANGE_TYPE_TOPIC, false, false, false, errmsg);
		std::string UniRoutingKey = getUniRoutingKey(channelUri, userId);
		UniRoutingKey = "wsrtc_vpclient_test_com.vpclient_test.dwk.broad";
		Listener.basicPublish(UniRoutingKey,  message.c_str(), errmsg);
		Listener.Disconnect();
	}
}

void BaseSignalAdapter::broadcastMessage(std::string channelUri, std::string message)
{
	if (m_pRabbitmq.get() != nullptr)
	{
		string errmsg;
		std::string BroadRoutingKey = getBroadRoutingKey(channelUri);
		m_pRabbitmq->basicPublish(BroadRoutingKey.c_str(), message.c_str(), errmsg);
	}
}

void BaseSignalAdapter::sendUnicastMessage(std::string  channelUri, std::string userId, std::string message)
{
	if (m_pRabbitmq.get() != nullptr)
	{
		string errmsg;
		std::string BroadRoutingKey = getBroadRoutingKey(channelUri);
		m_pRabbitmq->basicPublish(BroadRoutingKey.c_str(), message.c_str(), errmsg);
	}
}

void BaseSignalAdapter::SetMessageReceived(ThreadSignalListener mWSSignalListener)
{

	m_WSSignalListener = mWSSignalListener;
	m_pSignalListenerThread= std::make_shared<std::thread>([=]()
	{
		if (m_pRabbitmq.get() != nullptr)
		{
	
			m_bRun = true;
			while (m_bRun)
			{
				CRabbitMQ Listener;
				Listener.Connect();
				string errmsg;
				int get_number = 1;
				::timeval timeout = { 5,5 };
				//::timeval *timeout = NULL;
				vector<CMessage> message_array;
				//��RabbitMQ������ȡ��Ϣ
				if (Listener.consumer(m_strQueue, mWSSignalListener, false, &timeout, errmsg) < 0)
				{
					cout << "ȡ��Ϣʧ�ܣ�" << endl;
				}
				else
				{
					cout << "ȡ��Ϣ�ɹ���" << endl;
				}
				Listener.Disconnect();
			}
		
		
		}
	});
	m_pSignalListenerThread->detach();

	
}

//strutils.cpp
bool BaseSignalAdapter::startswith(const std::string& str, const std::string& start)
{
	int srclen = str.size();
	int startlen = start.size();
	if (srclen >= startlen)
	{
		std::string temp = str.substr(0, startlen);
		if (temp == start)
			return true;
	}

	return false;
}

// �滻�ַ�����ָ�����ַ���
int	BaseSignalAdapter::ReplaceStr(std::string &strBuf, std::string strSrc, std::string strDes)
{
	size_t				sBufSize = strBuf.size();
	char*				pStart = (char *)strBuf.c_str();
	char*				pEnd = pStart + sBufSize;
	std::string 		strReturn;
	int					nCount = 0;

	if (strBuf.empty())
	{
		return strBuf.size();
	}

	for (;; Sleep(1))
	{
		char*		pFind = strstr(pStart, (char *)strSrc.c_str());

		if (NULL == pFind)
		{
			strReturn.append(pStart);
			break;
		}

		nCount++;
		strReturn.append(pStart, pFind);
		strReturn.append(strDes);
		pStart = pFind + strSrc.size();

		if (pStart >= pEnd)
		{
			break;
		}
	}

	strBuf = strReturn;

	return nCount;

}

std::string BaseSignalAdapter::getUniRoutingKey(std::string url, std::string userId)
{
	if (url.size() < 1)
	{
		printf("url is empty");
		return "";
	}
	if (!startswith(url,"wssig://")) 
	{
		printf("url should start with wssig://");
		return "";
	}
	url = url.substr(8);
	ReplaceStr(url, ".", "_");
	ReplaceStr(url, "/", ".");
	std::string routingKey = url  + ".uni." + userId;
	return routingKey;
}

std::string BaseSignalAdapter::getMultiRoutingKey(std::string url, std::string groupId)
{
	if (url.size() < 1)
	{
		printf("url is empty");
		return "";
	}
	if (!startswith(url, "wssig://"))
	{
		printf("url should start with wssig://");
		return "";
	}
	url = url.substr(8);
	ReplaceStr(url, ".", "_");
	ReplaceStr(url, "/", ".");
	std::string routingKey = url + ".multi." + groupId;
	return routingKey;
}

/**
* @param url wssig://host/appid/channelId
* @return
*/
std::string BaseSignalAdapter::getBroadRoutingKey(std::string url) 
{
	if (url.size()<1)
	{
		printf("url is empty");
		return "";
	}
	if (!startswith(url, "wssig://"))
	{
		printf("url should start with wssig://");
		return "";
	}
	url = url.substr(8);
	ReplaceStr(url, ".", "_");
	ReplaceStr(url, "/", ".");
	std::string routingKey = url + ".broad" ;
	return routingKey;
}


