#pragma once

#include <windows.h>
#include <string.h>
#include <iostream>
#include <functional>

typedef std::function<void(std::string, std::string)> ThreadSignalListener;

class BaseSignalAdapter;

class  BaseSignal
{
public:
	BaseSignal();

	~BaseSignal();

	//  说明:初始化信令频道
	int Create(const std::string &host = "127.0.0.1",
		int port = 5672,
		const std::string &username = "guest",
		const std::string &password = "guest",
		const std::string &vhost = "/");

	//加入对应的频道
	int joinSignalChannel(std::string url, std::string userId, ThreadSignalListener mWSSignalListener=nullptr);
	
	//发送消息
	void sendMessage(std::string  channelUri, std::string  userId, std::string  message);

	//广播消息
	void broadcastMessage(std::string channelUri, std::string message);

	//  说明：退出信令频道，并销毁实例，之后无法接收和发送该频道消息。
	void quitSignalChannel();

private:

	BaseSignalAdapter  *m_Apater;
	int					m_nPort;
	std::string			m_strHost;
	std::string			m_strUserName;
	std::string			m_strPassWord;
	std::string			m_strVHost;

};

