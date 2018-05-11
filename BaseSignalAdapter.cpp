
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
	//m_pSignalListenerThread.reset();
	//m_pRabbitmq.reset();
	m_is_connected = false;
	m_pRabbitmqSend.reset();
}

BaseSignalAdapter::~BaseSignalAdapter()
{
	quitSignalChannel();
}

int32_t BaseSignalAdapter::Connect(const std::string & host, int port,
	const std::string & username,
	const std::string & password,
	const std::string & vhost, string &ErrorReturn)
{

	m_nPort = port;
	m_strHost = host;
	m_strUserName = username;
	m_strPassWord = password;
	m_strVHost = vhost;

	if (m_is_connected == true)
	{
		return -1;
	}
	m_pRabbitmqSend.reset(new CRabbitMQ);
	int res = m_pRabbitmqSend->Connect(m_strHost, m_nPort, m_strUserName, m_strPassWord, m_strVHost, ErrorReturn);
	if (res < 0)
	{
		m_pRabbitmqSend.reset();
	}
	m_pRabbitmqSend->setExchangeName(m_strExchangeName);
	return res;
}

int BaseSignalAdapter::joinSignalChannel(std::string url, std::string userId, ThreadSignalListener mWSSignalListener, std::string &ErrorReturn)
{
	m_strQueue = userId;


	std::shared_ptr<CRabbitMQ>	  Rabbitmq;
	Rabbitmq.reset(new CRabbitMQ);
	int res = Rabbitmq->Connect(m_strHost, m_nPort, m_strUserName, m_strPassWord, m_strVHost, ErrorReturn);
	if (res < 0)
	{
		Rabbitmq.reset();
	}
	if (Rabbitmq.get() != nullptr)
	{
	//	Rabbitmq->exchangeDeclare(m_strExchangeName, EXCHANGE_TYPE_TOPIC, false, false, false, ErrorReturn);
	//	Rabbitmq->queue_delete(userId, 1, ErrorReturn);
		int nres = Rabbitmq->queueDeclare(userId, false, false, false, false, ErrorReturn);
		if (nres != 0)
		{
			return nres;
		}
		std::string UniRoutingKey = getUniRoutingKey(url, userId);

		std::string BroadRoutingKey = getBroadRoutingKey(url);

		if (UniRoutingKey.size() > 0)
		{
			Rabbitmq->bindQueue(userId, m_strExchangeName, UniRoutingKey);
		}

		if (BroadRoutingKey.size() > 0)
		{
			Rabbitmq->bindQueue(userId, m_strExchangeName, BroadRoutingKey);
		}
		Rabbitmq->Disconnect();
		Rabbitmq.reset();

		//创建监听队列
		SetMessageReceived(mWSSignalListener);
		return 0;
	}
	else
	{
		return -1;
	}

	return 0;
}


int32_t BaseSignalAdapter::quitSignalChannel(std::string &ErrorReturn)
{
	string errmsg;
	if (m_bRun)
	{
		*m_bRun = false;
	}
	

	if (nullptr != m_pRabbitmqSend.get())
	{
		bool bStart = false;
		std::thread joinThread([=, &bStart]()
		{
			std::shared_ptr<CRabbitMQ>	  RabbitmqSend = m_pRabbitmqSend;
			bStart = true;
			if (RabbitmqSend)
			{
				RabbitmqSend->Disconnect();
				RabbitmqSend.reset();
			}			
		});
		while (!bStart)
		{
			Sleep(1);
		}
		m_pRabbitmqSend.reset();
		joinThread.detach();
	}
	
	return 0;
}

void BaseSignalAdapter::sendMessage(std::string  channelUri, std::string  userId, std::string  message)
{
	if (m_pRabbitmqSend.get() != nullptr)
	{
		string errmsg;

		std::string UniRoutingKey = getUniRoutingKey(channelUri, userId);
		m_pRabbitmqSend->basicPublish(UniRoutingKey, message.c_str(), errmsg);
	}
}

void BaseSignalAdapter::broadcastMessage(std::string channelUri, std::string message)
{
	if (m_pRabbitmqSend.get() != nullptr)
	{
		string errmsg;
		std::string BroadRoutingKey = getBroadRoutingKey(channelUri);
		m_pRabbitmqSend->basicPublish(BroadRoutingKey, message.c_str(), errmsg);
	}
}

void BaseSignalAdapter::sendUnicastMessage(std::string  channelUri, std::string userId, std::string message)
{
	if (m_pRabbitmqSend.get() != nullptr)
	{
		string errmsg;
		std::string BroadRoutingKey = getBroadRoutingKey(channelUri);
		m_pRabbitmqSend->basicPublish(BroadRoutingKey, message.c_str(), errmsg);
	}
}
std::shared_ptr<bool> BaseSignalAdapter::GetRunStatus()
{
	return m_bRun;
}
void BaseSignalAdapter::SetMessageReceived(ThreadSignalListener mWSSignalListener)
{
	m_WSSignalListener = mWSSignalListener;
	m_bRun.reset(new bool(true));
	m_is_connected = true;
	std::thread joinThread([=]()
	{
		std::weak_ptr<bool> bRun = GetRunStatus();
		if (!bRun.lock())
		{
			return;
		}
		std::string   strQueue = m_strQueue;
		std::string   strHost = m_strHost;
		std::string   strUserName = m_strUserName;
		std::string   strPassWord = m_strPassWord;
		std::string   strVHost = m_strVHost;
		int nPort = m_nPort;
		
		{
			
			while (bRun.lock() && *(bRun.lock()))
			{
				CRabbitMQ Listener;
				Listener.setChannel(2);
				int res = Listener.Connect(strHost, nPort, strUserName, strPassWord, strVHost);
				if (res != 0)
				{
					if (bRun.lock())
					{
						m_is_connected = false;
						Sleep(500);
						continue;
					}
				}
				if (bRun.lock())
				{
					m_is_connected = true;
				}
				string errmsg;
				int get_number = 1;
				while (bRun.lock() && *bRun.lock().get() == true)
				{
					::timeval timeout = { 0,100 };
					//::timeval *timeout = NULL;
					vector<CMessage> message_array;
					//从RabbitMQ服务器取消息
					if (Listener.consumer(bRun,strQueue, mWSSignalListener, false, false, true, false, &timeout,errmsg ) < 0)
					{
						Listener.Disconnect();
						cout << "取消息失败！" << endl;
						break;						
					}
					else
					{
						cout << "取消息成功！" << endl;
					}
				}
				Listener.Disconnect();

			}

			if (bRun.lock())
			{
				m_is_connected = false;
			}

			}
			

	});
	joinThread.detach();
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
	if (!startswith(url, "wssig://"))
	{
		printf("url should start with wssig://");
		return "";
	}
	url = url.substr(8);
	ReplaceStr(url, ".", "_");
	ReplaceStr(url, "/", ".");
	std::string routingKey = url + ".uni." + userId;
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
	std::string routingKey = url + ".broad";
	return routingKey;
}


