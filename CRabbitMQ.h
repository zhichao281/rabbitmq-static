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
*   @brief 消息队列的消息实体
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
	//拷贝构造函数
	CMessage(const CMessage &other)
	{
		this->m_data = other.m_data;
		this->m_durable = other.m_durable;
		this->m_type = other.m_type;
		this->m_routekey = other.m_routekey;
	}
	//重载 赋值= 运算符
	CMessage operator=(const CMessage &other)
	{
		if (this == &other) return *this;
		this->m_data = other.m_data;
		this->m_durable = other.m_durable;
		this->m_type = other.m_type;
		this->m_routekey = other.m_routekey;
		return *this;
	}

	string  m_data;             //消息内容，目前只支持文本，所以采用string
	int32_t m_durable;          //消息是否持久化; 1 不持久化; 2 持久化
	string  m_type;             //消息类型  目前只支持文本类型 "text/plain"
	string  m_routekey;         //消息路由 默认所有消息的路由都以 “msg.”开头
};


class CRabbitMQ
{
public:

	CRabbitMQ();

	~CRabbitMQ();

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
	
	/**
	* @brief quitSignalChannel ：退出信令频道，并销毁实例，之后无法接收和发送该频道消息。
	* @param [out] ErrorReturn   错误信息
	* @return 等于0值代表成功断开连接，小于0代表错误，错误信息从ErrorReturn返回
	*/
	int32_t Disconnect(std::string &ErrorReturn = std::string(""));

	//返回1成功，其他是错误
	int32_t AssertError(amqp_rpc_reply_t x, string context, string &ErrorReturn);
	
	/**
	*   @brief       exchange_declare   声明exchange
	*	@param       [in]               exchange       交换机实例
	*   @param        [out] ErrorReturn   错误信息
	*   @return 等于0值代表成功创建exchange，小于0代表错误，错误信息从ErrorReturn返回
	*/
	int32_t exchangeDeclare(
		const std::string &exchange_name,
		const std::string &exchange_type,
		bool passive = false,
		bool durable = false,
		bool auto_delete = false, string &ErrorReturn = string(""));

	/**
	*   @brief       queueDeclare                    声明消息队列
	*	@param       [in]    queue         消息队列实例
	*   @param       [out]   ErrorReturn   错误信息*/
	int32_t queueDeclare(
		const std::string &queue_name,
		bool passive = false,
		bool durable = false,
		bool exclusive = true,
		bool auto_delete = false, std::string &ErrorReturn = string(""));

	/**
	*   @brief       queue_bind                       将队列，交换机和绑定规则绑定起来形成一个路由表
	*	@param       [in]   queue         消息队列
	*	@param       [in]   exchange      交换机名称
	*	@param       [in]   bind_key      路由名称  “msg.#”“msg.weather.**”
	*   @param       [out]  ErrorReturn   错误信息
	*   @return 等于0值代表成功绑定，小于0代表错误，错误信息从ErrorReturn返回
	*   @deprecated 由于特殊的原因，这个函数可能会在将来的版本中取消。
	*/
	int32_t bindQueue(const std::string &queue_name,
		const std::string &exchange_name,
		const std::string &routing_key = "",
		string &ErrorReturn = string(""));

	/**
	*   @brief       unbindQueue                       将队列，交换机和绑定规则绑定起来形成一个路由表
	*	@param       [in]   queue         消息队列
	*	@param       [in]   exchange      交换机名称
	*	@param       [in]   bind_key      路由名称  “msg.#”“msg.weather.**”
	*   @param       [out]  ErrorReturn   错误信息
	*   @return 等于0值代表成功绑定，小于0代表错误，错误信息从ErrorReturn返回
	*   @deprecated 由于特殊的原因，这个函数可能会在将来的版本中取消。
	*/
	int32_t unbindQueue(const std::string &queue_name,
		const std::string &exchange_name,
		const std::string &routing_key = "",
		string &ErrorReturn = string(""));



	/**
	* @brief publish  发布消息
	* @param [in] messag         消息实体
	* @param [in] rout_key       路由规则
	*   1.Direct Exchange – 处理路由键。需要将一个队列绑定到交换机上，要求该消息与一个特定的路由键完全匹配。
	*   2.Fanout Exchange – 不处理路由键。将队列绑定到交换机上。一个发送到交换机的消息都会被转发到与该交换机绑定的所有队列上。
	*   3.Topic Exchange – 将路由键和某模式进行匹配。此时队列需要绑定要一个模式上。符号“#”匹配一个或多个词，符号“*”匹配不多不少一个词。
	*      因此“audit.#”能够匹配到“audit.irs.corporate”，但是“audit.*” 只会匹配到“audit.irs”
	* @param [out] ErrorReturn   错误信息
	* @return 等于0值代表成功发送消息实体个数，小于0代表发送错误，错误信息从ErrorReturn返回
	*/
	int32_t basicPublish(vector<CMessage> &message, string routing_key, string &ErrorReturn = string(""));

	int32_t basicPublish(CMessage &message, string routing_key, string &ErrorReturn = string(""));

	/**
	* @brief publish  发布消息
	* @param [in] messag         消息实体
	* @param [in] rout_key       路由规则
	*   1.Direct Exchange – 处理路由键。需要将一个队列绑定到交换机上，要求该消息与一个特定的路由键完全匹配。
	*   2.Fanout Exchange – 不处理路由键。将队列绑定到交换机上。一个发送到交换机的消息都会被转发到与该交换机绑定的所有队列上。
	*   3.Topic Exchange – 将路由键和某模式进行匹配。此时队列需要绑定要一个模式上。符号“#”匹配一个或多个词，符号“*”匹配不多不少一个词。
	*      因此“audit.#”能够匹配到“audit.irs.corporate”，但是“audit.*” 只会匹配到“audit.irs”
	* @param [out] ErrorReturn   错误信息
	* @return 等于0值代表成功发送消息实体个数，小于0代表发送错误，错误信息从ErrorReturn返回
	*/
	int32_t basicPublish(
		const std::string &routing_key, 
		const string &message, string &ErrorReturn = string(""));

	/**
	* @brief consumer  消费消息
	* @param [in]  queue        队列
	* @param [out] message      消息实体
	* @param [int] GetNum       需要取得的消息个数
	* @param [int] timeout      取得的消息是延迟，若为NULL，表示持续取，无延迟，阻塞状态
	* @param [out] ErrorReturn   错误信息
	* @return 等于0值代表成功，小于0代表错误，错误信息从ErrorReturn返回
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
	*   @brief       queue_delete                     删除消息队列。
	*	@param       [in]               queuename     消息队列名称
	*	@param       [in]               if_unused     消息队列是否在用，1 则论是否在用都删除
	*   @param       [out]              ErrorReturn   错误信息
	*   @return 等于0值代表成功删除queue，小于0代表错误，错误信息从ErrorReturn返回
	*/
	int32_t queue_delete(const string queuename, int32_t if_unused = 0, string &ErrorReturn = string(""));


	/**
	* @brief setChannel         设置通道号
	* @param [in]  channel      设置的通道号
	*/
	void setChannel(const uint32_t channel);

	/**
	* @brief getChannel    获得当前通道号
	* @return              返回当前通道号
	*/
	uint32_t getChannel()const;


	void __sleep(uint32_t millsecond);

	void  setExchangeName(std::string strExchangeNmae);
private:

	ThreadSignalListener		  m_WSSignalListener;  //监听amqo信令频道接受信息


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

