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
extern long long com_bit;

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

void database::recv_query_though_net(boost::asio::ip::tcp::socket& database_socket) {
    // First read the size of the incoming data
    vector<size_t> data_size(2);
    asio::read(database_socket, asio::buffer(data_size));
    size_t rows = data_size[0];
    size_t cols = data_size[1];
    // Read the actual data
    std::vector<int> flat_data(rows*cols);
    asio::read(database_socket, asio::buffer(flat_data));
    
    // Reshape the flat data back to 2D vector
    // Assuming we know the dimensions of the 2D vector
    
    
    // Check if the data size matches the expected size
    if (flat_data.size() != rows * cols) {
        throw std::runtime_error("Received data size does not match expected size");
    }
    
    // Fill the 2D vector
     database_recv_from_leader.reserve(rows); // 预分配行数
    for (int i = 0; i < rows; ++i) {
        // 直接构造每一行，避免多次resize
        database_recv_from_leader.emplace_back(flat_data.begin() + i * cols, flat_data.begin() + (i + 1) * cols);
    }
}

void database::recv_randomness_though_net(boost::asio::ip::tcp::socket& database_socket, int b) {
    // First read the size of the incoming data
    size_t data_size;
    asio::read(database_socket, asio::buffer(&data_size, sizeof(size_t)));
    // Read the actual data
    std::vector<int> data(data_size);
    asio::read(database_socket, asio::buffer(data));
    // Extract location_randomness (first b elements)
    location_randomness.assign(data.begin(), data.begin() + b);
    // cout << "\t recv chan size: " << data.size() << endl;
    // cout << "recv chan: " ;
    //             for (auto element : data) {
    //                 cout << element << " ";
    //             }
    //             cout << endl;
    // Check if we have targeted_randomness (if data size is 2*b + 1)
    if (data_size == 2 * b + 1) {
        // Extract targeted_randomness (next b elements)
        relatived_randomness.assign(data.begin() + b, data.begin() + 2 * b);
        // Extract global_randomness (last element)
        global_randomness = data.back();
    } else {
        // Only location_randomness and global_randomness are present
        global_randomness = data.back();
    }
}

void database::send_answer_to_leader(boost::asio::ip::tcp::socket& database_socket) {
    // 先发送数据大小
    size_t data_size = database_send_to_leader.size();
    asio::write(database_socket, asio::buffer(&data_size, sizeof(data_size)));
    
    // 再发送实际数据
    asio::write(database_socket, asio::buffer(database_send_to_leader));
    com_bit += data_size;
}
