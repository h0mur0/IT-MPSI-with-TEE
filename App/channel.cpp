#include <vector>
#include <iostream>
#include <type_traits>
#include "channel.h"
#include "Enclave_u.h"
#include "sgx_eid.h"

using namespace std;
extern sgx_enclave_id_t global_eid; 

extern long long com_bit;
// 基本模板：当 T 不是 vector 时，视作一个元素，返回 1
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

// 构造函数实现
channel::channel() {}

// leader 到 database 的数据传输
void channel::leader_to_database(leader& ld, database& db) {
    if (db.role == "base") {
        db.database_recv_from_leader = ld.leader_send_to_cb[db.client_id][db.database_id];
        com_bit += countElements(ld.leader_send_to_cb[db.client_id][db.database_id]);
    } else {
        db.database_recv_from_leader = ld.leader_send_to_tb[db.client_id][db.database_id];
        com_bit += countElements(ld.leader_send_to_tb[db.client_id][db.database_id]);
    }
}

// // client 到 database 的数据传输
// void channel::client_to_database(client& cl, database& db) {
//     db.database_recv_from_client = cl.client_send_to_database[db.database_id];
//     com_bit += countElements(cl.client_send_to_database[db.database_id]);
// }

// void channel::database_to_database(database& db1, database& db2) {
//     db2.database_recv_from_database.push_back(db1.database_send_to_database);
//     com_bit += countElements(db1.database_send_to_database);
// }

// database 到 leader 的数据传输
void channel::database_to_leader(database& db, leader& ld) {
    if (db.role == "base") {
        ld.leader_recv_from_cb[db.client_id][db.database_id] = db.database_send_to_leader;
        com_bit += countElements(db.database_send_to_leader);
    } 
    else {
        ld.leader_recv_from_tb[db.client_id][db.database_id] = db.database_send_to_leader;
        com_bit += countElements(db.database_send_to_leader);
    }
}
