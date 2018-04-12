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

	//  ˵��:��ʼ������Ƶ��
	int Create(const std::string &host = "127.0.0.1",
		int port = 5672,
		const std::string &username = "guest",
		const std::string &password = "guest",
		const std::string &vhost = "/");

	//�����Ӧ��Ƶ��
	int joinSignalChannel(std::string url, std::string userId, ThreadSignalListener mWSSignalListener=nullptr);
	
	//������Ϣ
	void sendMessage(std::string  channelUri, std::string  userId, std::string  message);

	//�㲥��Ϣ
	void broadcastMessage(std::string channelUri, std::string message);

	//  ˵�����˳�����Ƶ����������ʵ����֮���޷����պͷ��͸�Ƶ����Ϣ��
	void quitSignalChannel();

private:

	BaseSignalAdapter  *m_Apater;
	int					m_nPort;
	std::string			m_strHost;
	std::string			m_strUserName;
	std::string			m_strPassWord;
	std::string			m_strVHost;

};

