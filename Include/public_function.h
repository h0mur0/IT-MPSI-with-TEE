#ifndef PUBLIC_FUNCTION_H
#define PUBLIC_FUNCTION_H

#include <vector>
#include <tuple>
#include <map>
#include <iostream>
#include <cstdint>
#include <list>
#include <random> // 添加随机数生成头文件
#include <boost/beast.hpp>
#include <boost/asio.hpp>

using namespace std;
namespace asio = boost::asio;

template<typename T>
size_t countElements(const T&) {
    return 1;
}

// 偏特化：当传入的是 vector 时，递归累加其所有子元素的大小
template<typename T>
size_t countElements(const std::vector<T>& v) {
    size_t sum = 0;
    for (const auto& elem : v) {
        sum += countElements(elem);
    }
    return sum;
}

// 计算 a^b mod m
int mod_exp(int a, int b, int m);

// 生成一个长度为 K 的随机向量，每个元素在 [0, L-1] 范围内
vector<int> generate_random_vector(int L, int K);

// 计算两个向量的点积
int dot_product(const vector<int>& vec1, const vector<int>& vec2);

// 判断一个数是否为素数，使用 Miller-Rabin 算法
bool is_prime(int n, int k = 20);

// 查找大于 M 的下一个素数
int select_L(int M);

// 选择最优的领导者
int select_leader(const vector<vector<int>>& P, const vector<int>& N, const int& M);
void print_help();
void parse_args(int argc, char* argv[], int& M, vector<string>& fileNames, vector<int>& N, int& K);
void encode(const vector<string>& fileNames, vector<int>& Sk, map<string, int>& data2Sk, vector<vector<int>>& P, int& K);
void decode(const vector<int>& intersection, const map<string, int>& data2Sk, vector<string>& intersection_string);
void add_plain(const int* A, const int* B, int* C, size_t N, size_t b);
uint64_t fnv1a_64(const char* data, size_t len);
uint64_t murmur3_64(const char* key, uint64_t len, uint64_t seed);
class CuckooHashTableConsumer {
    private:
        int size;
        int maxSteps;
    
        int hash1(int key);
        int hash2(int key);
        int hash3(int key);
        std::vector<int> hashFunctions(int key);
        void rehash();
    
    public:
        CuckooHashTableConsumer(int initSize, int maxSteps = 1000);
        bool insert(int key);
        std::vector<int> table;
        bool search(int key);
        bool remove(int key);
        void display();
    };
    
class CuckooHashTableProducer {
private:
    int size;

    int hash1(int key);
    int hash2(int key);
    int hash3(int key);
    std::vector<int> hashFunctions(int key);

public:
    CuckooHashTableProducer(int size);
    std::vector<std::vector<int>> table;
    void insert(int key);
    void display();
};
void create_sockets(boost::asio::io_context& io_context, int m, int n,
                    std::vector<std::vector<boost::asio::ip::tcp::socket>>& leader_sockets_to_cb,
                    std::vector<std::vector<boost::asio::ip::tcp::socket>>& leader_sockets_to_tb,
                    std::vector<std::vector<boost::asio::ip::tcp::socket>>& tee_sockets_to_cb,
                    std::vector<std::vector<boost::asio::ip::tcp::socket>>& tee_sockets_to_tb,
                    std::vector<std::vector<boost::asio::ip::tcp::socket>>& cb_sockets_for_leader,
                    std::vector<std::vector<boost::asio::ip::tcp::socket>>& cb_sockets_for_tee,
                    std::vector<std::vector<boost::asio::ip::tcp::socket>>& tb_sockets_for_leader,
                    std::vector<std::vector<boost::asio::ip::tcp::socket>>& tb_sockets_for_tee);

void send_randomness_though_net(std::vector<std::vector<boost::asio::ip::tcp::socket>>& tee_sockets_to_cb,
                                std::vector<std::vector<boost::asio::ip::tcp::socket>>& tee_sockets_to_tb,
                                std::vector<int>& loc_buf,
                                std::vector<int>& rel_buf,
                                std::vector<int>& glo_buf,
                                int m, int n, int b);
void cleanup_all_sockets(
    std::vector<std::vector<boost::asio::ip::tcp::socket>>& leader_sockets_to_cb,
    std::vector<std::vector<boost::asio::ip::tcp::socket>>& leader_sockets_to_tb,
    std::vector<std::vector<boost::asio::ip::tcp::socket>>& tee_sockets_to_cb,
    std::vector<std::vector<boost::asio::ip::tcp::socket>>& tee_sockets_to_tb,
    std::vector<std::vector<boost::asio::ip::tcp::socket>>& cb_sockets_for_leader,
    std::vector<std::vector<boost::asio::ip::tcp::socket>>& cb_sockets_for_tee,
    std::vector<std::vector<boost::asio::ip::tcp::socket>>& tb_sockets_for_leader,
    std::vector<std::vector<boost::asio::ip::tcp::socket>>& tb_sockets_for_tee);
// void recv_randomness_though_net(boost::asio::ip::tcp::socket& database_socket, database& database_);

#endif // PUBLIC_FUNCTION_H

