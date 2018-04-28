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
	* @brief connect  连接消息队列服务器
	* @param [out] ErrorReturn   错误信息
	* @return 等于0值代表成功连接，小于0代表错误，错误信息从ErrorReturn返回
	*/
	int32_t Connect(const std::string &host = "127.0.0.1",
		int port = 5672,
		const std::string &username = "guest",
		const std::string &password = "guest",
		const std::string &vhost = "/", string &ErrorReturn = string(""));

	//加入对应的频道
	int joinSignalChannel(std::string url, std::string userId, ThreadSignalListener mWSSignalListener, std::string &ErrorReturn = string(""));

	//发送消息
	void sendMessage(std::string  channelUri, std::string  userId, std::string  message);

	//广播消息
	void broadcastMessage(std::string channelUri, std::string message);

	void sendUnicastMessage(std::string  channelUri, std::string userId, std::string msg);

	/**
	* @brief quitSignalChannel ：退出信令频道，并销毁实例，之后无法接收和发送该频道消息。
	* @param [out] ErrorReturn   错误信息
	* @return 等于0值代表成功断开连接，小于0代表错误，错误信息从ErrorReturn返回
	*/
	int32_t quitSignalChannel(std::string &ErrorReturn = std::string(""));

	/**
	* 单播消息：host/path/uni/userid
	* @param url    wssig://host/appid/channelId
	* @param userId 要发的对象
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

	// 替换字符串中指定的字符串
	int	ReplaceStr(std::string &strBuf, std::string strSrc, std::string strDes);

	//字符段判断初始字符
	bool startswith(const std::string& str, const std::string& start);

	//设置回调信息
	void SetMessageReceived(ThreadSignalListener mWSSignalListener);


private:
	//std::shared_ptr<CRabbitMQ>	  m_pRabbitmq;

	std::shared_ptr<CRabbitMQ>	  m_pRabbitmqSend;
	ThreadSignalListener		  m_WSSignalListener;  //监听amqo信令频道接受信息


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

