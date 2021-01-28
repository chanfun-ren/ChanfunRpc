#pragma once
#include <string>
#include <memory>
#include <zmq.hpp>
#include <functional>
#include <iostream>
#include <sstream>
#include "Serialize.hpp"
#include <unordered_map>
#include <map>



class RpcClient {

private:
    // ChanfunRpc rpc_;
    // zmq通信环境
    zmq::context_t context_;
    // 指向zmq::socket_t的指针，自定义deleter
    std::unique_ptr<zmq::socket_t, std::function<void(zmq::socket_t*)>> socket_;

public:
    // 接收RpcServer的地址信息，构造RpcClient
    RpcClient(std::string ip, int port) : context_(1) {
        auto deleter = [](zmq::socket_t* sock){ sock->close(); delete sock; sock = nullptr;};
        socket_ = std::unique_ptr<zmq::socket_t, std::function<void(zmq::socket_t*)>> {new zmq::socket_t(context_, ZMQ_REQ), deleter};
        std::string addr = "tcp://";
        addr += ip;
        addr += ":";
        addr += std::to_string(port);
        socket_->connect(addr);
    }
    ~RpcClient() {
	    context_.close();
    }

    // 向服务端请求建立连接, 这里可以并入构造函数
    // void connect(string ip, int port) {}

    // 设置超时时间
    // void set_timeout(uint32_t time) {
    //     socket_->setsockopt(ZMQ_RCVTIMEO, time);
    // }

    // client, call函数, 调用远程服务接口, 
    // 函数内部封装序列化服务id和相关参数
	template<typename R, typename... Params>
	R call(std::string service_id, Params... ps) {
		using args_type = std::tuple<typename std::decay<Params>::type...>;
		args_type args = std::make_tuple(ps...);

        std::ostringstream seq;
        // 序列化nameid，output至seq, 
        // 序列化args至seq
        Serialize(seq, service_id);
        Serialize(seq, args);
		
		return send_recv<R>(seq);
	}

	template<typename R>
	R call(std::string& service_id) {
		std::ostringstream seq;
        Serialize(seq, service_id);
        return send_recv<R>(seq);
	}

    // net_call接口，通过网络模块将序列化后的服务id以及参数递交给RpcServer，
    // 同时返回远程服务的返回值ret
    template<typename R>
    std::enable_if_t<!std::is_same_v<R, void>, R>
    send_recv(std::ostringstream& oss) {
        // 1. 将otringstream对象转化成zeromq可接受的数据格式
        // 通过zeromq::socket_t 发送序列化后的数据

        socket_->send(zmq::buffer(oss.str()));

        // 2. 等待server回传服务返回值，
        // 通过zeromq::socket_t接收
        zmq::message_t reply{};
        socket_->recv(reply);

        // 3. 利用reply构造出istringstream对象, 进行反序列化
        std::istringstream iss(reply.to_string());
        R ret;
        Deserialize(iss, ret);
        return ret;
    }

    template<typename R>
    std::enable_if_t<std::is_same_v<R, void>, void>     // 返回类型为void
    send_recv(std::ostringstream& oss) {
        // 1. 将otringstream对象转化成zeromq可接受的数据格式
        // 通过zeromq::socket_t 发送序列化后的数据

        socket_->send(zmq::buffer(oss.str()));

        // 2. 等待server回传服务返回值，
        // 通过zeromq::socket_t接收
        zmq::message_t reply{};
        socket_->recv(reply);
        
        return ;
    }

};

