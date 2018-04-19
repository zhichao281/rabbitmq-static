
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
		m_pRabbitmq->queue_delete(m_strQueue, 0, errmsg);
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
		//创建exchange
		//m_pRabbitmq->exchangeDeclare(m_strExchangeName, EXCHANGE_TYPE_TOPIC, false, true, false, ErrorReturn);
		//先删除队列
		int32_t nres=m_pRabbitmq->queue_delete(userId, 0, ErrorReturn);
		if (nres !=0)
		{
			return nres;
		}
		//每个客户端使用一个私有的队列
		nres=m_pRabbitmq->queueDeclare(userId, false, false, true, false, ErrorReturn);
		if (nres != 0)
		{
			return nres;
		}
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

void BaseSignalAdapter::sendMessage(std::string  channelUri, std::string  userId, std::string  message)
{
	if (m_pRabbitmq.get() != nullptr)
	{
		string errmsg;

		std::string UniRoutingKey = getUniRoutingKey(channelUri, userId);
		m_pRabbitmq->basicPublish(UniRoutingKey, message.c_str(), errmsg);

		////创建exchange
		//CRabbitMQ Listener;
		//Listener.Connect("180.97.246.16", 6841, "vpclient_test+wsrtc.vpclient_test.com+dwk000", "1523526883053_43a5ee3de54bb08d9aa12e1c82c1a9259f6f0ad6", "wsrtc.vpclient_test.com");
		////Listener.exchangeDeclare(m_strExchangeName, EXCHANGE_TYPE_TOPIC, false, false, false, errmsg);
		//std::string UniRoutingKey = getUniRoutingKey(channelUri, userId);
		//Listener.basicPublish(UniRoutingKey,  message.c_str(), errmsg);
		//Listener.Disconnect();
	}
}

void BaseSignalAdapter::broadcastMessage(std::string channelUri, std::string message)
{
	if (m_pRabbitmq.get() != nullptr)
	{
		string errmsg;
		std::string BroadRoutingKey = getBroadRoutingKey(channelUri);
		m_pRabbitmq->basicPublish(BroadRoutingKey, message.c_str(), errmsg);
	}
}

void BaseSignalAdapter::sendUnicastMessage(std::string  channelUri, std::string userId, std::string message)
{
	if (m_pRabbitmq.get() != nullptr)
	{
		string errmsg;
		std::string BroadRoutingKey = getBroadRoutingKey(channelUri);
		m_pRabbitmq->basicPublish(BroadRoutingKey, message.c_str(), errmsg);
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
		//	CRabbitMQ Listener;
		//	Listener.setChannel(m_pRabbitmq->getChannel()+1);

		//	Listener.Connect("180.97.246.16", 6841, "vpclient_test+wsrtc.vpclient_test.com+dwk000", "1523526883053_43a5ee3de54bb08d9aa12e1c82c1a9259f6f0ad6", "wsrtc.vpclient_test.com");
			string errmsg;
			int get_number = 1;
			while (m_bRun)
			{		
				//::timeval timeout = { 5,5 };
				::timeval *timeout = NULL;
				vector<CMessage> message_array;
				//从RabbitMQ服务器取消息
				if (m_pRabbitmq->consumer(m_strQueue, mWSSignalListener, false,false,false,true, timeout, errmsg) < 0)
				{
					cout << "取消息失败！" << endl;
				}
				else
				{
					cout << "取消息成功！" << endl;
				}			
			}
			//Listener.Disconnect();
		
		
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

// 替换字符串中指定的字符串
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


