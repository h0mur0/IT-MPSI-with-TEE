#ifndef DATABASE_H
#define DATABASE_H

#include <vector>

#include "public_function.h"

class database {
public:
    std::string filename;  // 文件名
    std::vector<int> P_db;
    std::string role; 
    std::string state;
    int client_id;  // 数据库的客户端ID
    int database_id;  // 数据库ID
    int global_randomness;
    std::vector<int> database_send_to_leader;
    std::vector<int> location_randomness;  // 来自客户端的接收数据
    std::vector<int> relatived_randomness;  // 来自其他数据库的接收数据
    std::vector<std::vector<int>> database_recv_from_leader;  // 来自leader的接收数据
    std::vector<std::vector<int>> incidence_vectors; 


    // 构造函数，初始化数据库相关属性
    database() = default;
    database(string filename, int client_id, int database_id, string role);

    // void create_and_send_relatived_randomness(int L, int b, int M, int N);

    // 创建并发送回复
    void create_and_send_reply(int L, int b, int N);
    void preprocessing(int L, int b, int eta);
    void database::recv_query_though_net(tcp::socket& database_socket,
                                     std::vector<std::vector<int>>& database_recv_from_leader);
};

#endif // DATABASE_H

