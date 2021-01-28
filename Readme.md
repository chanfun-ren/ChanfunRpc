# ChanfunRpc

由C++17编写，网络模块依赖于zmq，因此在编译时注意开启`-std=c++17`，以及链接zmq。

使用示例：
1. 请求服务端(client)
```cpp
#include "RpcClient.hpp"

int main() {

    RpcClient rpc_client("127.0.0.1", 5555);
    auto ret1 = rpc_client.call<int>("add", 2, 3);
    std::cout << "rpc add :" << ret1 << std::endl;

    auto ret2 = rpc_client.call<int>("minus", 1000, 1);
    std::cout << "rpc minus : " << ret2 << std::endl;

    return 0;
}

```

2. 提供服务端(server)
```cpp
#include "RpcServer.hpp"

int add(int a, int b) {
    std::cout << "a + b = " << a + b << std::endl;
    return a + b;
}


struct Foo {
    int minus(int a, int b) {
        std::cout << "a - b = " << a - b << std::endl;
        return a - b;
    }
};


int main() {
    RpcServer rpc_server(5555);
    
    rpc_server.regist("add", add);
    Foo foo;
    rpc_server.regist("minus", &Foo::minus, &foo);
   

    rpc_server.run();
    
    return 0;
}

```