#include "BaseSignal.h"

#include "BaseSignalAdapter.h"

BaseSignal::BaseSignal()
{
	m_Apater = new BaseSignalAdapter();
	m_nPort=5672;
	m_strHost= "127.0.0.1";
	m_strUserName="guest";
	m_strPassWord="guest";
	m_strVHost="/";
}


BaseSignal::~BaseSignal()
{
	delete m_Apater;
	m_Apater = nullptr;
}

int BaseSignal::Create(const std::string &host,
	int port,
	const std::string &username,
	const std::string &password,
	const std::string &vhost)
{

	m_nPort = port;
	m_strHost = host;
	m_strUserName = username;
	m_strPassWord = password;
	m_strVHost = vhost;
	m_Apater->Connect(host, port, username, password, vhost);
	return 0;
}

int BaseSignal::joinSignalChannel(std::string url, std::string userId, ThreadSignalListener mWSSignalListener)
{
	int res = m_Apater->joinSignalChannel(url, userId, mWSSignalListener);
	return res;
}

void BaseSignal::sendMessage(std::string  channelUri, std::string  userId, std::string  message)
{
	m_Apater->sendMessage(channelUri, userId, message);
}

void BaseSignal::broadcastMessage(std::string  channelUri, std::string  message)
{
	m_Apater->broadcastMessage(channelUri, message);
}

void BaseSignal::quitSignalChannel()
{
	m_Apater->quitSignalChannel();
}

