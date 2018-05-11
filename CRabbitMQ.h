#pragma once

#include "amqp.h"
#include <string.h>
#include <iostream>
#include <list>
#include <memory>
#include <thread>
#include <vector>
using namespace std;

typedef std::function<void(std::string, std::string)> ThreadSignalListener;


/**
*   @brief ��Ϣ���е���Ϣʵ��
*
*   class Message in "MessageBody.h"
**/
class CMessage
{
public:
	CMessage(string data, int32_t durable = 1) :
		m_data(data), m_durable(durable)
	{
		this->m_type = "text/plain";
	}
	CMessage(string data, string routekey, int32_t durable = 2) :
		m_data(data), m_routekey(routekey), m_durable(durable)
	{
		this->m_type = "text/plain";
	}
	//�������캯��
	CMessage(const CMessage &other)
	{
		this->m_data = other.m_data;
		this->m_durable = other.m_durable;
		this->m_type = other.m_type;
		this->m_routekey = other.m_routekey;
	}
	//���� ��ֵ= �����
	CMessage operator=(const CMessage &other)
	{
		if (this == &other) return *this;
		this->m_data = other.m_data;
		this->m_durable = other.m_durable;
		this->m_type = other.m_type;
		this->m_routekey = other.m_routekey;
		return *this;
	}

	string  m_data;             //��Ϣ���ݣ�Ŀǰֻ֧���ı������Բ���string
	int32_t m_durable;          //��Ϣ�Ƿ�־û�; 1 ���־û�; 2 �־û�
	string  m_type;             //��Ϣ����  Ŀǰֻ֧���ı����� "text/plain"
	string  m_routekey;         //��Ϣ·�� Ĭ��������Ϣ��·�ɶ��� ��msg.����ͷ
};


class CRabbitMQ
{
public:

	CRabbitMQ();

	~CRabbitMQ();

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
	
	/**
	* @brief quitSignalChannel ���˳�����Ƶ����������ʵ����֮���޷����պͷ��͸�Ƶ����Ϣ��
	* @param [out] ErrorReturn   ������Ϣ
	* @return ����0ֵ����ɹ��Ͽ����ӣ�С��0������󣬴�����Ϣ��ErrorReturn����
	*/
	int32_t Disconnect(std::string &ErrorReturn = std::string(""));

	//����1�ɹ��������Ǵ���
	int32_t AssertError(amqp_rpc_reply_t x, string context, string &ErrorReturn);
	
	/**
	*   @brief       exchange_declare   ����exchange
	*	@param       [in]               exchange       ������ʵ��
	*   @param        [out] ErrorReturn   ������Ϣ
	*   @return ����0ֵ����ɹ�����exchange��С��0������󣬴�����Ϣ��ErrorReturn����
	*/
	int32_t exchangeDeclare(
		const std::string &exchange_name,
		const std::string &exchange_type,
		bool passive = false,
		bool durable = false,
		bool auto_delete = false, string &ErrorReturn = string(""));

	/**
	*   @brief       queueDeclare                    ������Ϣ����
	*	@param       [in]    queue         ��Ϣ����ʵ��
	*   @param       [out]   ErrorReturn   ������Ϣ*/
	int32_t queueDeclare(
		const std::string &queue_name,
		bool passive = false,
		bool durable = false,
		bool exclusive = true,
		bool auto_delete = false, std::string &ErrorReturn = string(""));

	/**
	*   @brief       queue_bind                       �����У��������Ͱ󶨹���������γ�һ��·�ɱ�
	*	@param       [in]   queue         ��Ϣ����
	*	@param       [in]   exchange      ����������
	*	@param       [in]   bind_key      ·������  ��msg.#����msg.weather.**��
	*   @param       [out]  ErrorReturn   ������Ϣ
	*   @return ����0ֵ����ɹ��󶨣�С��0������󣬴�����Ϣ��ErrorReturn����
	*   @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/
	int32_t bindQueue(const std::string &queue_name,
		const std::string &exchange_name,
		const std::string &routing_key = "",
		string &ErrorReturn = string(""));

	/**
	*   @brief       unbindQueue                       �����У��������Ͱ󶨹���������γ�һ��·�ɱ�
	*	@param       [in]   queue         ��Ϣ����
	*	@param       [in]   exchange      ����������
	*	@param       [in]   bind_key      ·������  ��msg.#����msg.weather.**��
	*   @param       [out]  ErrorReturn   ������Ϣ
	*   @return ����0ֵ����ɹ��󶨣�С��0������󣬴�����Ϣ��ErrorReturn����
	*   @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/
	int32_t unbindQueue(const std::string &queue_name,
		const std::string &exchange_name,
		const std::string &routing_key = "",
		string &ErrorReturn = string(""));



	/**
	* @brief publish  ������Ϣ
	* @param [in] messag         ��Ϣʵ��
	* @param [in] rout_key       ·�ɹ���
	*   1.Direct Exchange �C ����·�ɼ�����Ҫ��һ�����а󶨵��������ϣ�Ҫ�����Ϣ��һ���ض���·�ɼ���ȫƥ�䡣
	*   2.Fanout Exchange �C ������·�ɼ��������а󶨵��������ϡ�һ�����͵�����������Ϣ���ᱻת������ý������󶨵����ж����ϡ�
	*   3.Topic Exchange �C ��·�ɼ���ĳģʽ����ƥ�䡣��ʱ������Ҫ��Ҫһ��ģʽ�ϡ����š�#��ƥ��һ�������ʣ����š�*��ƥ�䲻�಻��һ���ʡ�
	*      ��ˡ�audit.#���ܹ�ƥ�䵽��audit.irs.corporate�������ǡ�audit.*�� ֻ��ƥ�䵽��audit.irs��
	* @param [out] ErrorReturn   ������Ϣ
	* @return ����0ֵ����ɹ�������Ϣʵ�������С��0�����ʹ��󣬴�����Ϣ��ErrorReturn����
	*/
	int32_t basicPublish(vector<CMessage> &message, string routing_key, string &ErrorReturn = string(""));

	int32_t basicPublish(CMessage &message, string routing_key, string &ErrorReturn = string(""));

	/**
	* @brief publish  ������Ϣ
	* @param [in] messag         ��Ϣʵ��
	* @param [in] rout_key       ·�ɹ���
	*   1.Direct Exchange �C ����·�ɼ�����Ҫ��һ�����а󶨵��������ϣ�Ҫ�����Ϣ��һ���ض���·�ɼ���ȫƥ�䡣
	*   2.Fanout Exchange �C ������·�ɼ��������а󶨵��������ϡ�һ�����͵�����������Ϣ���ᱻת������ý������󶨵����ж����ϡ�
	*   3.Topic Exchange �C ��·�ɼ���ĳģʽ����ƥ�䡣��ʱ������Ҫ��Ҫһ��ģʽ�ϡ����š�#��ƥ��һ�������ʣ����š�*��ƥ�䲻�಻��һ���ʡ�
	*      ��ˡ�audit.#���ܹ�ƥ�䵽��audit.irs.corporate�������ǡ�audit.*�� ֻ��ƥ�䵽��audit.irs��
	* @param [out] ErrorReturn   ������Ϣ
	* @return ����0ֵ����ɹ�������Ϣʵ�������С��0�����ʹ��󣬴�����Ϣ��ErrorReturn����
	*/
	int32_t basicPublish(
		const std::string &routing_key, 
		const string &message, string &ErrorReturn = string(""));

	/**
	* @brief consumer  ������Ϣ
	* @param [in]  queue        ����
	* @param [out] message      ��Ϣʵ��
	* @param [int] GetNum       ��Ҫȡ�õ���Ϣ����
	* @param [int] timeout      ȡ�õ���Ϣ���ӳ٣���ΪNULL����ʾ����ȡ�����ӳ٣�����״̬
	* @param [out] ErrorReturn   ������Ϣ
	* @return ����0ֵ����ɹ���С��0������󣬴�����Ϣ��ErrorReturn����
	*/
	int32_t consumer(const string & queue_name, vector<CMessage> &message, bool durable = false, uint32_t GetNum = 1, struct timeval *timeout = NULL, string &ErrorReturn = string(""));
	
	int32_t consumer(const string & queue_name, vector<string> &message_array, bool durable = false, uint32_t GetNum = 1, struct timeval *timeout = NULL, string &ErrorReturn = string(""));
	
	int32_t consumer(std::weak_ptr<bool> bRun, const string & queue_name,
		std::function<void(std::string, std::string)> SignalListener,
		bool durable = false,
		bool no_local = false,
		bool no_ack = false,
		bool exclusive = false,
		struct timeval *timeout = NULL, string &ErrorReturn = string("") );


	/**
	*   @brief       queue_delete                     ɾ����Ϣ���С�
	*	@param       [in]               queuename     ��Ϣ��������
	*	@param       [in]               if_unused     ��Ϣ�����Ƿ����ã�1 �����Ƿ����ö�ɾ��
	*   @param       [out]              ErrorReturn   ������Ϣ
	*   @return ����0ֵ����ɹ�ɾ��queue��С��0������󣬴�����Ϣ��ErrorReturn����
	*/
	int32_t queue_delete(const string queuename, int32_t if_unused = 0, string &ErrorReturn = string(""));


	/**
	* @brief setChannel         ����ͨ����
	* @param [in]  channel      ���õ�ͨ����
	*/
	void setChannel(const uint32_t channel);

	/**
	* @brief getChannel    ��õ�ǰͨ����
	* @return              ���ص�ǰͨ����
	*/
	uint32_t getChannel()const;


	void __sleep(uint32_t millsecond);

	void  setExchangeName(std::string strExchangeNmae);
private:

	ThreadSignalListener		  m_WSSignalListener;  //����amqo����Ƶ��������Ϣ


	std::shared_ptr<std::thread>  m_pSignalListenerThread;
public:
	std::string   m_strExchangeName;
	std::string   m_strQueue;
	int			  m_nPort;
	std::string	  m_strHost;
	std::string   m_strUserName;
	std::string   m_strPassWord;
	std::string   m_strVHost;
	amqp_socket_t           *m_sock;
	amqp_connection_state_t  m_conn;
	uint32_t				 m_channel;

	volatile bool  m_bRun;
};

