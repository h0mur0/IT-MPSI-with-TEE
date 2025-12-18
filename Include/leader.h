#ifndef LEADER_H
#define LEADER_H
#include <map>
#include <vector>
#include "public_function.h"

class leader {
public:
    std::string filename;  // 文件名
    std::vector<int> P_leader;
    std::vector<std::vector<std::vector<std::vector<int>>>> control_queries;
    std::vector<std::vector<std::vector<std::vector<int>>>> targeted_queries;
    std::vector<std::vector<std::vector<std::vector<int>>>> leader_send_to_cb;
    std::vector<std::vector<std::vector<std::vector<int>>>> leader_send_to_tb;
    std::vector<std::vector<std::vector<int>>> leader_recv_from_cb;
    std::vector<std::vector<std::vector<int>>> leader_recv_from_tb;
    std::vector<std::vector<int>> schedule_hash_tables;
    int leader_id;

    // 构造函数声明
    leader() = default;
    leader(string filename, int leader_id, int M, int N, int b);

    // 成员函数声明
    void send_query();
    std::vector<int> calculate_intersection(int M, int N, int b, int L);
    void preprocessing(int N, int M, int b, int eta, int L);
    void send_query_though_net(std::vector<std::vector<boost::asio::ip::tcp::socket>>& leader_sockets_to_cb,
                                   std::vector<std::vector<boost::asio::ip::tcp::socket>>& leader_sockets_to_tb,
                                   int m, int n);
    void recv_answer_from_databases(
    std::vector<std::vector<boost::asio::ip::tcp::socket>>& cb_sockets_for_leader,
    std::vector<std::vector<boost::asio::ip::tcp::socket>>& tb_sockets_for_leader,
    int m, int n);
};

#endif // LEADER_H

