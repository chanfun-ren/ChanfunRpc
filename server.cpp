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
