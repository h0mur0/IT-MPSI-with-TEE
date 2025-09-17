#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include "leader.h"
#include <numeric>
#include <cmath>
#include <chrono> // 包含计时功能
#include <algorithm>
#include <thread>
#include <fstream>
#include <random>
#include <string>
#include <unordered_set>
#include <omp.h>
#include "sgx_eid.h"

using namespace std;
extern sgx_enclave_id_t global_eid; 

// 构造函数实现
leader::leader(string filename, int leader_id, int M, int N, int b)
    : leader_id(leader_id) {
        ifstream file(filename);
        string line;
        while (getline(file, line)) {
            // 处理行中的空格，可能需要去掉前后空格
            string data = line;
            data.erase(0, data.find_first_not_of(" \t"));
            data.erase(data.find_last_not_of(" \t") + 1);
            P_leader.push_back(stoi(data));
        }
        
        leader_recv_from_cb = vector<vector<vector<int>>>(M, vector<vector<int>>(N));
        leader_recv_from_tb = vector<vector<vector<int>>>(M, vector<vector<int>>(N));
        control_queries = vector<vector<vector<vector<int>>>>(M, vector<vector<vector<int>>>(N, vector<vector<int>>(b, vector<int>(3))));
        targeted_queries = vector<vector<vector<vector<int>>>>(M, vector<vector<vector<int>>>(N, vector<vector<int>>(b, vector<int>(3))));
        schedule_hash_tables = vector<vector<int>>(N);
    }

void leader::preprocessing(int N, int M, int b, int eta, int L) {
    vector<CuckooHashTableProducer> producer_tables(N, CuckooHashTableProducer(b));
    vector<CuckooHashTableConsumer> consumer_tables(N, CuckooHashTableConsumer(b));
    vector<vector<vector<int>>> index_hash_tables(N);
    // 计时
    auto t_start = chrono::high_resolution_clock::now();
    for (int j = 0; j < N; j++){
        int start = j * eta;
        int end = (j + 1) * eta;
        for (int k = start; k < end; k++) {
            producer_tables[j].insert(k);
            consumer_tables[j].insert(k);
        }
        schedule_hash_tables[j] = consumer_tables[j].table;
        index_hash_tables[j] = producer_tables[j].table;
    }
    auto t_end = chrono::high_resolution_clock::now();
    long long total_us = chrono::duration_cast<chrono::microseconds>(t_end - t_start).count();
    cout << "预处理1: " << total_us << " 微秒" << endl;
    unordered_set<int> pset(P_leader.begin(), P_leader.end());
    vector<vector<vector<int>>> pre_query_vectors(N, vector<vector<int>>(b, vector<int>(3, 0)));
    for (int j = 0; j < N; j++) {
        for (int ell = 0; ell < b; ell++) {
                int tmp = schedule_hash_tables[j][ell];
                if (tmp == -1) continue; // 跳过未使用的槽位             
                if (pset.count(tmp)) {
                    int k = find(index_hash_tables[j][ell].begin(),index_hash_tables[j][ell].end(),tmp) - index_hash_tables[j][ell].begin();
                    pre_query_vectors[j][ell][k] = 1;             
            }
        }
    } 
    //计时
    auto t_end1 = chrono::high_resolution_clock::now();
    long long total_us1 = chrono::duration_cast<chrono::microseconds>(t_end1 - t_end).count();
    cout << "预处理2: " << total_us1 << " 微秒" << endl;
    vector<int> control_query;
    vector<int> targeted_query;
     thread_local static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, L - 1);

    // 2) 并行化三重循环（需编译时加 -fopenmp）
    #pragma omp parallel for collapse(3) schedule(static)
    for (int i = 0; i < M - 1; i++) {
        for (int j = 0; j < N; j++) {
            for (int ell = 0; ell < b; ell++) {
                // 3) 直接生成并写入三元数组，避免 vector 分配/复制
                auto &slot = control_queries[i][j][ell];
                slot[0] = dist(rng);
                slot[1] = dist(rng);
                slot[2] = dist(rng);
            }
        }
    }
    // for (int i = 0; i < M - 1; i++) {
    //     for (int j = 0; j < N; j++) {
    //         for (int ell = 0; ell < b; ell++) {
    //             // 生成控制查询
    //             control_query = generate_random_vector(L, 3);   
    //             control_queries[i][j][ell] = control_query; 
    //         }}}
    auto t_end2 = chrono::high_resolution_clock::now();
    long long total_us2 = chrono::duration_cast<chrono::microseconds>(t_end2 - t_end1).count();
    cout << "预处理3: " << total_us2 << " 微秒" << endl;
    targeted_queries = control_queries;           
    for (int i = 0; i < M - 1; i++) {
        for (int j = 0; j < N; j++) {
            for (int ell = 0; ell < b; ell++) {
                // 生成目标查询
                for (int k = 0; k < 3; k++) {
                    targeted_queries[i][j][ell][k] += pre_query_vectors[j][ell][k];  // 将查询与预处理向量结合
                }
            }}}
                
    auto t_end3 = chrono::high_resolution_clock::now();
    long long total_us3 = chrono::duration_cast<chrono::microseconds>(t_end3 - t_end2).count();
    cout << "预处理4: " << total_us3 << " 微秒" << endl;
}

// 创建并发送查询
void leader::send_query() {
    leader_send_to_cb = control_queries;  // 控制查询发送给控制数据库
    leader_send_to_tb = targeted_queries;  // 目标查询发送给目标数据库
}

// 计算交集
vector<int> leader::calculate_intersection(int M, int N, int b, int L) {
    vector<int> intersection;
    unordered_set<int> pset(P_leader.begin(), P_leader.end());
    for (int j = 0; j < N; j++) {
        for (int ell = 0; ell < b; ell++) {
            int element = schedule_hash_tables[j][ell];
            if (element == -1) continue; // 跳过未使用的槽位
            if (pset.count(element) == 0) continue; // 如果元素不在 P_leader 中，跳过
            vector<int> z;
            for (int i = 0; i < M - 1; i++) {
                z.push_back(leader_recv_from_tb[i][j][ell] - leader_recv_from_cb[i][j][ell]);
            }
            int e = accumulate(z.begin(), z.end(), 0) % L;
            // cout << e << " ";
            if (e == 0) {
                intersection.push_back(element);
            }
        }
    }
     // cout << endl;
    return intersection;
}

void leader::send_query_though_net(const std::vector<std::vector<std::vector<std::vector<int>>>>& leader_send_to_cb,
                                   const std::vector<std::vector<std::vector<std::vector<int>>>>& leader_send_to_tb,
                                   const std::vector<std::vector<tcp::acceptor>>& leader_sockets_to_cb,
                                   const std::vector<std::vector<tcp::acceptor>>& leader_sockets_to_tb,
                                   int m, int n) {
    std::vector<std::thread> threads;
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            threads.emplace_back([&, i, j]() {
                std::vector<int> flat_data;
                for (const auto& vec : leader_send_to_cb[i][j]) {
                    flat_data.insert(flat_data.end(), vec.begin(), vec.end());
                }
                asio::write(leader_sockets_to_cb[i][j], asio::buffer(flat_data));
            });
        }
    }
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            threads.emplace_back([&, i, j]() {
                std::vector<int> flat_data;
                for (const auto& vec : leader_send_to_tb[i][j]) {
                    flat_data.insert(flat_data.end(), vec.begin(), vec.end());
                }
                asio::write(leader_sockets_to_tb[i][j], asio::buffer(flat_data));
            });
        }
    }
    for (auto& t : threads) {
        t.join();
    }
}