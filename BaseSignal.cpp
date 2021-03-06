#include "BaseSignal.h"

#include "BaseSignalAdapter.h"

BaseSignal::BaseSignal()
{
	m_Apater = new BaseSignalAdapter();

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
	const std::string &vhost, std::string &ErrorReturn)
{
	
	int res= m_Apater->Connect(host, port, username, password, vhost, ErrorReturn);
	return res;
}

int BaseSignal::joinSignalChannel(std::string url, std::string userId, ThreadSignalListener mWSSignalListener, std::string &ErrorReturn )
{
	int res = m_Apater->joinSignalChannel(url, userId, mWSSignalListener, ErrorReturn);
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

