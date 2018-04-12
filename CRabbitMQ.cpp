#include "CRabbitMQ.h"
#include "amqp_tcp_socket.h"
#include "amqp_tcp_socket.h"
#include <windows.h>
#include <thread>

#include <Ctime>
#include <time.h>

#define BROKER_HEARTBEAT 0

CRabbitMQ::CRabbitMQ()
{

	m_pSignalListenerThread.reset();
	//m_nPort = 5672;
	//m_strHost = "127.0.0.1";
	//m_strUserName = "guest";
	//m_strPassWord = "guest";
	//m_strVHost = "/";

	m_nPort = 6841;
	m_strHost = "180.97.246.16";
	m_strUserName = "vpclient_test+wsrtc.vpclient_test.com+dwk000";
	m_strPassWord = "1523526883053_43a5ee3de54bb08d9aa12e1c82c1a9259f6f0ad6";
	m_strVHost = "wsrtc.vpclient_test.com";


	this->m_channel = 1; //默认用1号通道，通道无所谓 
	m_sock = NULL;
	m_conn = NULL;
}

CRabbitMQ::~CRabbitMQ()
{
	m_nPort = 0;
	m_strHost = "";
	string errmsg;
	if (NULL != m_conn)
	{
		Disconnect(errmsg);
		m_conn = NULL;
	}
	m_pSignalListenerThread.reset();
}

int32_t CRabbitMQ::Connect(const std::string & host, int port,
	const std::string & username,
	const std::string & password,
	const std::string & vhost, string &ErrorReturn)
{

	m_nPort = port;
	m_strHost = host;
	m_strUserName = username;
	m_strPassWord = password;
	m_strVHost = vhost;

	this->m_conn = amqp_new_connection();
	if (NULL == m_conn)
	{
		ErrorReturn = "无法获得连接";
		return -1;
	}
	m_sock = amqp_tcp_socket_new(m_conn);
	if (NULL == m_sock)
	{
		ErrorReturn = "无法获得套接字";
		return -2;
	}
	int status = amqp_socket_open(m_sock, m_strHost.c_str(), m_nPort);
	if (status < 0)
	{
		ErrorReturn = "无法连接目标主机";
		return -3;
	}

	if (1 == AssertError(amqp_login(m_conn, m_strVHost.c_str(),
		0, 131072, BROKER_HEARTBEAT, AMQP_SASL_METHOD_PLAIN,
		m_strUserName.c_str(), m_strPassWord.c_str()), "Loging in", ErrorReturn))
		return 0;
	else
		return -4;
}



int32_t CRabbitMQ::Disconnect(string &ErrorReturn)
{
	if (NULL != m_conn)
	{
		if (1 != AssertError(amqp_connection_close(m_conn, AMQP_REPLY_SUCCESS), "Closing connection", ErrorReturn))
			return -1;

		if (amqp_destroy_connection(m_conn) < 0)
			return -1;

		m_conn = NULL;
	}
	return 0;
}




//返回1代表正常 其他都是错
int32_t CRabbitMQ::AssertError(amqp_rpc_reply_t x, string context, string &ErrorReturn)
{
	char rtnmsg[1024];
	switch (x.reply_type) {
	case AMQP_RESPONSE_NORMAL:
		return 1;

	case AMQP_RESPONSE_NONE:
		sprintf(rtnmsg, "%s: missing RPC reply type!\n", context.c_str());
		break;

	case AMQP_RESPONSE_LIBRARY_EXCEPTION:
		sprintf(rtnmsg, "%s: %s\n", context.c_str(), amqp_error_string2(x.library_error));
		break;

	case AMQP_RESPONSE_SERVER_EXCEPTION:
		switch (x.reply.id) {
		case AMQP_CONNECTION_CLOSE_METHOD: {
			amqp_connection_close_t *m = (amqp_connection_close_t *)x.reply.decoded;
			sprintf(rtnmsg, "%s: server connection error %d, message: %.*s\n",
				context.c_str(),
				m->reply_code,
				(int)m->reply_text.len, (char *)m->reply_text.bytes);
			break;
		}
		case AMQP_CHANNEL_CLOSE_METHOD: {
			amqp_channel_close_t *m = (amqp_channel_close_t *)x.reply.decoded;
			sprintf(rtnmsg, "%s: server channel error %d, message: %.*s\n",
				context.c_str(),
				m->reply_code,
				(int)m->reply_text.len, (char *)m->reply_text.bytes);
			break;
		}
		default:
			sprintf(rtnmsg, "%s: unknown server error, method id 0x%08X\n", context, x.reply.id);
			break;
		}
		break;
	}
	ErrorReturn = rtnmsg;
	return -1;
}

//step1 declare an exchange
int32_t CRabbitMQ::exchangeDeclare(
	const std::string &exchange_name,
	const std::string &exchange_type,
	bool passive,
	bool durable,
	bool auto_delete, string &ErrorReturn)
{

	m_strExchangeName = exchange_name;
	//创建exchange
	amqp_channel_open(m_conn, m_channel);

	//amqp_confirm_select(m_conn, m_channel);  //在通道上打开Publish确认  


	amqp_bytes_t _exchange = amqp_cstring_bytes(exchange_name.c_str());
	amqp_bytes_t _type = amqp_cstring_bytes(exchange_type.c_str());
	int32_t  _passive = passive;
	int32_t  _durable = durable;      //交换机是否持久化
	int32_t  _auto_delete = auto_delete;      //交换机是否持久化
	int32_t _internal = false;



	amqp_exchange_declare(m_conn, m_channel, _exchange, _type, _passive, _durable, _auto_delete, _internal,amqp_empty_table);


	if (1 != AssertError(amqp_get_rpc_reply(m_conn), "exchange_declare", ErrorReturn))
	{
		amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}

	amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
	return 0;
}

//step2 declare a queue
int32_t CRabbitMQ::queueDeclare(
	const std::string &queue_name,
	bool passive,
	bool durable,
	bool exclusive,
	bool auto_delete, string &ErrorReturn)
{
	m_strQueue = queue_name;
	amqp_channel_open(m_conn, m_channel);
	amqp_confirm_select(m_conn, m_channel);  //在通道上打开Publish确认  
	amqp_bytes_t _queue = amqp_cstring_bytes(queue_name.c_str());
	int32_t _passive = passive;
	int32_t _durable = durable;
	int32_t _exclusive = exclusive;
	int32_t _auto_delete = auto_delete;
	amqp_queue_declare(m_conn, m_channel, _queue, _passive, _durable, _exclusive, _auto_delete, amqp_empty_table);
	if (1 != AssertError(amqp_get_rpc_reply(m_conn), "queue_declare", ErrorReturn))
	{
		amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}
	amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
}



//step3 bind
int32_t CRabbitMQ::bindQueue(
	const std::string & queue_name,
	const std::string & exchange_name,
	const std::string & routing_key,
	string & ErrorReturn)
{
	amqp_channel_open(m_conn, m_channel);
	amqp_confirm_select(m_conn, m_channel);  //在通道上打开Publish确认  
	amqp_bytes_t _queue = amqp_cstring_bytes(queue_name.c_str());
	amqp_bytes_t _exchange = amqp_cstring_bytes(exchange_name.c_str());
	amqp_bytes_t _routkey = amqp_cstring_bytes(routing_key.c_str());
	amqp_queue_bind(m_conn, m_channel, _queue, _exchange, _routkey, amqp_empty_table);
	if (1 != AssertError(amqp_get_rpc_reply(m_conn), "queue_bind", ErrorReturn))
	{
		amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}
	amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
	return 0;
}


int32_t  CRabbitMQ::unbindQueue(
	const std::string & queue_name,
	const std::string & exchange_name,
	const std::string & routing_key,
	string & ErrorReturn)
{
	amqp_channel_open(m_conn, m_channel);
	amqp_confirm_select(m_conn, m_channel);  //在通道上打开Publish确认  
	amqp_bytes_t _queue = amqp_cstring_bytes(queue_name.c_str());
	amqp_bytes_t _exchange = amqp_cstring_bytes(exchange_name.c_str());
	amqp_bytes_t _routkey = amqp_cstring_bytes(routing_key.c_str());
	amqp_queue_unbind(m_conn, m_channel, _queue, _exchange, _routkey, amqp_empty_table);
	if (1 != AssertError(amqp_get_rpc_reply(m_conn), "queue_unbind", ErrorReturn))
	{
		amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}
	amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
	return 0;
}

int32_t CRabbitMQ::basicPublish(vector<CMessage>& message, string routing_key, string & ErrorReturn)
{

	if (NULL == m_conn)
	{
		ErrorReturn = "还未创建连接";
		return -1;
	}
	amqp_channel_open(m_conn, m_channel);
	if (1 != AssertError(amqp_get_rpc_reply(m_conn), "open channel", ErrorReturn))
	{
		amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}

	amqp_basic_properties_t props;
	vector<CMessage>::iterator it;
	for (it = message.begin(); it != message.end(); ++it)
	{
		amqp_bytes_t message_bytes;
		message_bytes.len = (*it).m_data.length();
		message_bytes.bytes = (void *)((*it).m_data.c_str());
		props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
		props.content_type = amqp_cstring_bytes((*it).m_type.c_str());
		props.delivery_mode = (*it).m_durable; /* persistent delivery mode */

		amqp_bytes_t _exchange = amqp_cstring_bytes(m_strExchangeName.c_str());
		amqp_bytes_t _rout_key = amqp_cstring_bytes(routing_key.c_str());
		//printf("message: %.*s\n",(int)message_bytes.len,(char *)message_bytes.bytes);
		amqp_confirm_select(m_conn, m_channel);  //在通道上打开Publish确认  
		if (amqp_basic_publish(m_conn, m_channel, _exchange, _rout_key, 0, 0, &props, message_bytes) != 0)
		{
			//printf("发送消息失败。");
			if (1 != AssertError(amqp_get_rpc_reply(m_conn), "amqp_basic_publish", ErrorReturn))
			{
				amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
				return -1;
			}
		}
		//printf("message: %.*s\n",(int)message_bytes.len,(char *)message_bytes.bytes);
	}
	amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
	return 0;
}

int32_t CRabbitMQ::basicPublish(CMessage & message, string routing_key, string & ErrorReturn)
{
	vector<CMessage> msg;
	msg.push_back(message);
	return basicPublish(msg, routing_key, ErrorReturn);
}

int32_t CRabbitMQ::basicPublish(const std::string & routing_key, const string & message, string & ErrorReturn)
{
	CMessage msg(message);
	msg.m_durable = 1;
	return basicPublish(msg, routing_key, ErrorReturn);
}



//返回0是成功 否则全是失败
int32_t CRabbitMQ::consumer(const string & queue_name, vector<CMessage> &message, bool durable,uint32_t GetNum, struct timeval *timeout, string &ErrorReturn)
{
	if (NULL == m_conn)
	{
		ErrorReturn = "还未创建连接";
		return -1;
	}
	amqp_channel_open(m_conn, m_channel);
	amqp_confirm_select(m_conn, m_channel);  //在通道上打开Publish确认  
	if (1 != AssertError(amqp_get_rpc_reply(m_conn), "open channel", ErrorReturn))
	{
		amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}
	amqp_bytes_t queuename = amqp_cstring_bytes(queue_name.c_str());
	amqp_queue_declare(m_conn, m_channel, queuename, 0, durable, 0, 0, amqp_empty_table);

	amqp_basic_qos(m_conn, m_channel, 0, GetNum, 0);
	int ack = 0; // no_ack    是否需要确认消息后再从队列中删除消息
	amqp_basic_consume(m_conn, m_channel, queuename, amqp_empty_bytes, 0, ack, 0, amqp_empty_table);

	if (1 != AssertError(amqp_get_rpc_reply(m_conn), "Consuming", ErrorReturn))
	{
		amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}

	CMessage tmp("tmp");
	amqp_rpc_reply_t res;
	amqp_envelope_t envelope;
	int hasget = 0;
	while (GetNum > 0)
	{
		amqp_maybe_release_buffers(m_conn);
		res = amqp_consume_message(m_conn, &envelope, timeout, 0);
		if (AMQP_RESPONSE_NORMAL != res.reply_type)
		{
			amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
			ErrorReturn = "无法取得消息\n";
			if (0 == hasget)
				return -res.reply_type;
			else
				return hasget;
		}

		string str((char *)envelope.message.body.bytes, (char *)envelope.message.body.bytes + envelope.message.body.len);
		tmp.m_data = str;
		tmp.m_data = tmp.m_data.substr(0, (int)envelope.message.body.len);
		tmp.m_routekey = (char *)envelope.routing_key.bytes;
		tmp.m_routekey = tmp.m_routekey.substr(0, (int)envelope.routing_key.len);
		message.push_back(tmp);
		//delete p;
		amqp_destroy_envelope(&envelope);
		int rtn = amqp_basic_ack(m_conn, m_channel, envelope.delivery_tag, 1);
		if (rtn != 0)
		{
			amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
			return -1;
		}
		GetNum--;
		hasget++;
		__sleep(1);
	}

	return hasget;
}

int32_t CRabbitMQ::consumer(const string & queue_name, vector<string> &message_array, bool durable,uint32_t GetNum, struct timeval *timeout, string &ErrorReturn)
{
	if (NULL == m_conn)
	{
		ErrorReturn = "还未创建连接";
		return -1;
	}
	amqp_channel_open(m_conn, m_channel);
	amqp_confirm_select(m_conn, m_channel);  //在通道上打开Publish确认  
	if (1 != AssertError(amqp_get_rpc_reply(m_conn), "open channel", ErrorReturn))
	{
		amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}
	amqp_bytes_t queuename = amqp_cstring_bytes(queue_name.c_str());
	amqp_queue_declare(m_conn, m_channel, queuename, 0, 1, 0, 0, amqp_empty_table);

	amqp_basic_qos(m_conn, m_channel, 0, GetNum, 0);
	int ack = 0; // no_ack    是否需要确认消息后再从队列中删除消息
	amqp_basic_consume(m_conn, m_channel, queuename, amqp_empty_bytes, 0, ack, 0, amqp_empty_table);

	if (1 != AssertError(amqp_get_rpc_reply(m_conn), "Consuming", ErrorReturn))
	{
		amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}
	amqp_rpc_reply_t res;
	amqp_envelope_t envelope;
	int hasget = 0;
	while (GetNum > 0)
	{
		amqp_maybe_release_buffers(m_conn);
		res = amqp_consume_message(m_conn, &envelope, timeout, 0);
		if (AMQP_RESPONSE_NORMAL != res.reply_type)
		{
			amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
			ErrorReturn = "无法取得消息\n";
			if (0 == hasget)
				return -res.reply_type;
			else
				return hasget;
		}
		string str((char *)envelope.message.body.bytes, (char *)envelope.message.body.bytes + envelope.message.body.len);
		message_array.push_back(str);
		amqp_destroy_envelope(&envelope);
		int rtn = amqp_basic_ack(m_conn, m_channel, envelope.delivery_tag, 1);
		if (rtn != 0)
		{
			amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
			return -1;
		}
		GetNum--;
		hasget++;
		__sleep(1);
	}
	return hasget;
}

int32_t CRabbitMQ::consumer(const string & queue_name, std::function<void(std::string, std::string)> SignalListener, bool durable, timeval * timeout, string & ErrorReturn)
{
	if (NULL == m_conn)
	{
		ErrorReturn = "还未创建连接";
		return -1;
	}
	int GetNum = 1;
	int  channel = m_channel+1;
	amqp_channel_open(m_conn, channel);
	amqp_confirm_select(m_conn, channel);  //在通道上打开Publish确认  
	if (1 != AssertError(amqp_get_rpc_reply(m_conn), "open channel", ErrorReturn))
	{
		amqp_channel_close(m_conn, channel, AMQP_REPLY_SUCCESS);
		return -1;
	}
	amqp_bytes_t queuename = amqp_cstring_bytes(queue_name.c_str());
	amqp_queue_declare(m_conn, channel, queuename, 0, 0, 0, 0, amqp_empty_table);

	amqp_basic_qos(m_conn, channel, 0, GetNum, 0);
	int ack = 0; // no_ack    是否需要确认消息后再从队列中删除消息
	amqp_basic_consume(m_conn, channel, queuename, amqp_empty_bytes, 0, ack, 0, amqp_empty_table);

	if (1 != AssertError(amqp_get_rpc_reply(m_conn), "Consuming", ErrorReturn))
	{
		amqp_channel_close(m_conn, channel, AMQP_REPLY_SUCCESS);
		return -1;
	}
	amqp_rpc_reply_t res;
	amqp_envelope_t envelope;
	int hasget = 0;
	while (1)
	{
		amqp_maybe_release_buffers(m_conn);
		res = amqp_consume_message(m_conn, &envelope, timeout, 0);
		if (AMQP_RESPONSE_NORMAL != res.reply_type)
		{
			amqp_channel_close(m_conn, channel, AMQP_REPLY_SUCCESS);
			ErrorReturn = "无法取得消息\n";
			if (0 == hasget)
				return -res.reply_type;
			else
				return hasget;
		}
		string str((char *)envelope.message.body.bytes, (char *)envelope.message.body.bytes + envelope.message.body.len);
		
		SignalListener(queue_name, str);
		amqp_destroy_envelope(&envelope);
		int rtn = amqp_basic_ack(m_conn, channel, envelope.delivery_tag, 1);
		if (rtn != 0)
		{
			amqp_channel_close(m_conn, channel, AMQP_REPLY_SUCCESS);
			return -1;
		}
		hasget++;
		__sleep(1);
	}
	return hasget;
}


int32_t CRabbitMQ::queue_delete(const string queuename, int32_t if_unused, string &ErrorReturn)
{
	if (NULL == m_conn)
	{
		ErrorReturn = "还未创建连接";
		return -1;
	}
	amqp_channel_open(m_conn, m_channel);
	if (1 != AssertError(amqp_get_rpc_reply(m_conn), "open channel", ErrorReturn))
	{
		amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}

	amqp_queue_delete(m_conn, m_channel, amqp_cstring_bytes(queuename.c_str()), if_unused, 0);
	if (1 != AssertError(amqp_get_rpc_reply(m_conn), "delete queue", ErrorReturn))
	{
		amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}
	amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
	return 0;
}

void CRabbitMQ::__sleep(uint32_t millsecond)
{

#if defined (__linux)
	usleep(millsecond);
#elif defined (WIN32)
	Sleep(millsecond);
#endif
}
