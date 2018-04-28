#pragma once


#include "amqp.h"
#include <string.h>
#include <iostream>
#include <list>
#include <memory>
#include <thread>
#include "CRabbitMQ.h"
using namespace std;

typedef std::function<void(std::string, std::string)> ThreadSignalListener;

class BaseSignalAdapter
{
public:

	BaseSignalAdapter();

	~BaseSignalAdapter();

	/**
	* @brief connect  ������Ϣ���з�����
	* @param [out] ErrorReturn   ������Ϣ
	* @return ����0ֵ����ɹ����ӣ�С��0������󣬴�����Ϣ��ErrorReturn����
	*/
	int32_t Connect(const std::string &host = "127.0.0.1",
		int port = 5672,
		const std::string &username = "guest",
		const std::string &password = "guest",
		const std::string &vhost = "/", string &ErrorReturn = string(""));

	//�����Ӧ��Ƶ��
	int joinSignalChannel(std::string url, std::string userId, ThreadSignalListener mWSSignalListener, std::string &ErrorReturn = string(""));

	//������Ϣ
	void sendMessage(std::string  channelUri, std::string  userId, std::string  message);

	//�㲥��Ϣ
	void broadcastMessage(std::string channelUri, std::string message);

	void sendUnicastMessage(std::string  channelUri, std::string userId, std::string msg);

	/**
	* @brief quitSignalChannel ���˳�����Ƶ����������ʵ����֮���޷����պͷ��͸�Ƶ����Ϣ��
	* @param [out] ErrorReturn   ������Ϣ
	* @return ����0ֵ����ɹ��Ͽ����ӣ�С��0������󣬴�����Ϣ��ErrorReturn����
	*/
	int32_t quitSignalChannel(std::string &ErrorReturn = std::string(""));

	/**
	* ������Ϣ��host/path/uni/userid
	* @param url    wssig://host/appid/channelId
	* @param userId Ҫ���Ķ���
	* @return
	*/
	std::string getUniRoutingKey(std::string url, std::string userId);


	/**
	* @param url     wssig://host/appid/channelId
	* @param groupId
	* @return
	*/
	std::string getMultiRoutingKey(std::string url, std::string groupId);

	/**
	* @param url wssig://host/appid/channelId
	* @return
	*/
	std::string getBroadRoutingKey(std::string url);

	// �滻�ַ�����ָ�����ַ���
	int	ReplaceStr(std::string &strBuf, std::string strSrc, std::string strDes);

	//�ַ����жϳ�ʼ�ַ�
	bool startswith(const std::string& str, const std::string& start);

	//���ûص���Ϣ
	void SetMessageReceived(ThreadSignalListener mWSSignalListener);


private:
	//std::shared_ptr<CRabbitMQ>	  m_pRabbitmq;

	std::shared_ptr<CRabbitMQ>	  m_pRabbitmqSend;
	ThreadSignalListener		  m_WSSignalListener;  //����amqo����Ƶ��������Ϣ


	//std::shared_ptr<std::thread>  m_pSignalListenerThread;

	//std::shared_ptr<std::thread>  m_pSignalCreateThread;
	std::shared_ptr<bool> GetRunStatus();
public:
	std::string   m_strExchangeName;
	std::string   m_strQueue;

	int			  m_nPort;
	std::string	  m_strHost;
	std::string   m_strUserName;
	std::string   m_strPassWord;
	std::string   m_strVHost;

	std::shared_ptr<bool>  m_bRun;
	volatile bool  m_is_connected;
};

