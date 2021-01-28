#pragma once
#include <string>
#include <memory>
#include <zmq.hpp>
#include <functional>
#include <iostream>
#include <sstream>
#include <type_traits>
#include "Serialize.hpp"
#include <unordered_map>
#include <map>



class RpcServer {

private:
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t, std::function<void(zmq::socket_t*)>> socket_;
    // 通过接受std::istream类型的参数来屏蔽不同函数签名的差异, std::ostream用来接收返回值
    std::map< std::string, std::function<void(std::istream&, std::ostream&)> > service_;

public:
    // 指定port，构造RpcServer
    RpcServer(int port) : context_(1) {
        auto deleter = [](zmq::socket_t* sock){ sock->close(); delete sock; sock = nullptr;};
        socket_ = std::unique_ptr<zmq::socket_t, std::function<void(zmq::socket_t*)>> {new zmq::socket_t(context_, ZMQ_REP), deleter};
        std::string addr = "tcp://*:";
        addr += std::to_string(port);
        socket_->bind(addr);
    }

    ~RpcServer() {
        context_.close();
    }
    
    // 提供给server的注册接口
    template <typename F>   // 普通函数指针，lambda表达式，函数对象注册接口
    void regist(std::string service_id, F func) {
        service_[service_id] = std::bind(&RpcServer::uniform_<F>, this, func, std::placeholders::_1, std::placeholders::_2);
    }

    template <typename F, typename O>   // 类成员函数注册接口
    void regist(std::string service_id, F func, O* obj) {
        service_[service_id] = std::bind(&RpcServer::uniform_<F, O>, this, func, obj, std::placeholders::_1, std::placeholders::_2);
    }
    

    // uniform_接口，配合std::binder统一server本地调用形式，
    template <typename F>   // 非类成员函数可调用对象
    void uniform_(F func, std::istream& is, std::ostream& os) {
        helper_(func, is, os);
    }

    template <typename F, typename O>   // 类成员函数
    void uniform_(F func, O* obj, std::istream& is, std::ostream& os) {
        helper_(func, obj, is, os);
    }


    // helper_内部帮助接口
    // 普通函数指针，lambda表达式，函数对象，统一为std::function<>处理
    template <typename R, typename... Args>     
    void helper_(R(*func)(Args...), std::istream& is, std::ostream& os) {
        helper_(std::function<R(Args...)>(func), is, os);
    }

    // std::function<>
    template <typename R, typename... Args>     // std::function<R(Args...)>
    std::enable_if_t<std::is_same_v<R, void>, int>          // 没有返回值   
    helper_(std::function<R(Args...)> func, std::istream& is, std::ostream& os) {
        // 1. 获取参数的tuple类型
        using ArgsType = std::tuple<Args...>;

        // 2. 反序列化出参数tuple
        ArgsType params;
        Deserialize(is, params);

        // 3. 实际调用 （函数没有返回值）
        std::apply(func, params);
        return 0;
    }
    template <typename R, typename... Args>
    std::enable_if_t<!std::is_same_v<R, void>, double>     // 有返回值
    helper_(std::function<R(Args...)> func, std::istream& is, std::ostream& os) {
        // 1. 获取参数的tuple类型
        using ArgsType = std::tuple<Args...>;

        // 2. 反序列化出参数tuple
        ArgsType params;
        Deserialize(is, params);

        // 3. 实际调用 （函数有返回值）
        R ret = std::apply(func, params);
        Serialize(os, ret);

        return 0.0;
    } 


    // 类成员函数
    template<typename R, typename C, typename O, typename... Args>
    std::enable_if_t<std::is_same_v<R, void>, int>     // 无返回值      
    helper_(R(C::* func)(Args...), O* obj, std::istream& is, std::ostream& os) {
        auto classfunc = [=](Args... args) {
            (obj->*func)(args...);
        };

        // 1. 获取参数的tuple类型
        using ArgsType = std::tuple<Args...>;

        // 2. 反序列化出参数tuple
        ArgsType params;
        Deserialize(is, params);

        // 3. 实际调用 （函数没有返回值）
        std::apply(classfunc, params);

        return 0;
    }

    template<typename R, typename C, typename O, typename... Args>
    std::enable_if_t<!std::is_same_v<R, void>, double>     // 有返回值      
    helper_(R(C::* func)(Args...), O* obj, std::istream& is, std::ostream& os) {
        auto classfunc = [=](Args... args) -> R {
            return (obj->*func)(args...);
        };

        // 1. 获取参数的tuple类型
        using ArgsType = std::tuple<Args...>;

        // 2. 反序列化出参数tuple
        ArgsType params;
        Deserialize(is, params);

        // 3. 实际调用 （函数有返回值）
        R ret = std::apply(classfunc, params);
        Serialize(os, ret);
        
        return 0.0;
    }


    void recv_send() {
        // 1. 通过zmq网络模块接收client发送的请求
        zmq::message_t request;
        socket_->recv(request);
        
        // 2. 转化为istringstream对象，反序列化解析出服务id
        std::istringstream iss(request.to_string());
        std::string service_id;
        Deserialize(iss, service_id);

        // 3. 根据注册中心，找到对应可调用对象，传入参数进行调用，存储返回值ret

        // 这里刚开始的思路一直是先确定返回值ret类型，再序列化返ret，最后zmq向client发送序列
        // 这一块实际上很难确定ret类型。
        // 可以尝试换个思路：fun(iss, oss)，将类型确定的任务托管给fun，oss存储返回值返回序列
        std::ostringstream oss; 
        auto fun = service_[service_id];
        fun(iss, oss);

        // 4. 通过zmq向client发送ret序列，如果没有返回值发送空序列
        socket_->send(zmq::buffer(oss.str()));

    }


    void run() {
        while (1) {
            recv_send();
        }
    }


};

