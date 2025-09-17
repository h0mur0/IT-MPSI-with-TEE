#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <unordered_set>
#include <fstream>

#include "database.h"
#include "Enclave_u.h"
#include "sgx_eid.h"

using namespace std;
extern sgx_enclave_id_t global_eid; 

// 构造函数实现
database::database(string filename, int client_id, int database_id, string role)
    :client_id(client_id), database_id(database_id), role(role) {
        ifstream file(filename);
        string line;
        while (getline(file, line)) {
            // 处理行中的空格，可能需要去掉前后空格
            string data = line;
            data.erase(0, data.find_first_not_of(" \t"));
            data.erase(data.find_last_not_of(" \t") + 1);
            P_db.push_back(stoi(data));
        }
    }

void database::preprocessing(int L, int b, int eta) {
    // 预处理步骤
    CuckooHashTableProducer producer_table(b);
    int j = database_id;
    int start = j * eta;
    int end = (j + 1)  * eta;
    for (int k = start; k < end; k++) {
        producer_table.insert(k);
    }
    auto index_hash_table = producer_table.table;
    incidence_vectors = vector<vector<int>>(b, vector<int>(3, 0));
    std::unordered_set<int> pset(P_db.begin(), P_db.end());
    for (int ell = 0; ell < b; ell++) {
        for (int k = 0; k < 3; k++) {
            int tmp = index_hash_table[ell][k];
            if (pset.count(tmp)) {
                incidence_vectors[ell][k] = 1;
            }
        }
    }
}

// 创建并发送回复
void database::create_and_send_reply(int L, int b, int N) {
    if (role == "not base") {
        for (int ell = 0; ell < b; ell++) {
            int reply = global_randomness * (dot_product(incidence_vectors[ell], database_recv_from_leader[ell]) + location_randomness[ell] + relatived_randomness[ell]) % L;
            database_send_to_leader.push_back(reply);
        }
    }
    else {
        for (int ell = 0; ell < b; ell++) {
            int reply = global_randomness * (dot_product(incidence_vectors[ell], database_recv_from_leader[ell]) + location_randomness[ell]) % L;
            database_send_to_leader.push_back(reply);
        }
    }    
}

void database::recv_query_though_net(tcp::socket& database_socket,
                                     std::vector<std::vector<int>>& database_recv_from_leader) {
    std::vector<int> flat_data(database_recv_from_leader.size() * database_recv_from_leader[0].size());
    asio::read(database_socket, asio::buffer(flat_data));
    int index = 0;
    for (auto& row : database_recv_from_leader) {
        for (auto& elem : row) {
            elem = flat_data[index++];
        }
    }
}