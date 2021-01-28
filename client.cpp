#include "RpcClient.hpp"


int main() {

    RpcClient rpc_client("127.0.0.1", 5555);
    auto ret1 = rpc_client.call<int>("add", 2, 3);
    std::cout << "rpc add :" << ret1 << std::endl;

    auto ret2 = rpc_client.call<int>("minus", 1000, 1);
    std::cout << "rpc minus : " << ret2 << std::endl;

    return 0;
}
