/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <stdio.h>
#include <string.h>
#include <assert.h>

# include <unistd.h>
# include <pwd.h>
# define MAX_PATH FILENAME_MAX

#include "sgx_urts.h"
#include "App.h"
#include "Enclave_u.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <cmath>
#include <cstdlib>
#include <map>
#include <memory>
#include <chrono>
#include <algorithm>
#include <unordered_set>
#include <ctime>
#include <iomanip>
#include <thread>
#include <mutex>

#include "sgx_eid.h"
#include "client.h"
#include "leader.h"
#include "database.h"
#include "channel.h"
#include "public_function.h"

using namespace std;

// 全局
int M;                                // 参与方数量
int leader_id, sp_id;                
vector<string> fileNames;            
vector<int> N;                       
int K, L, b, eta;                    
vector<client> clients;              
vector<vector<database>> control_databases;
vector<vector<database>> targeted_databases;
leader ld;
shared_ptr<channel> chan;            
long long com_bit = 0;
ofstream outFile("output_PBC.txt", ios::app);
mutex file_mutex;                     // 同步写文件

// 单个线程的计时结果
struct ThreadTiming {
    string name;
    long long microseconds;
};
vector<ThreadTiming> timings;
mutex timing_mutex;

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {
        SGX_ERROR_UNEXPECTED,
        "Unexpected error occurred.",
        NULL
    },
    {
        SGX_ERROR_INVALID_PARAMETER,
        "Invalid parameter.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_MEMORY,
        "Out of memory.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_LOST,
        "Power transition occurred.",
        "Please refer to the sample \"PowerTransition\" for details."
    },
    {
        SGX_ERROR_INVALID_ENCLAVE,
        "Invalid enclave image.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ENCLAVE_ID,
        "Invalid enclave identification.",
        NULL
    },
    {
        SGX_ERROR_INVALID_SIGNATURE,
        "Invalid enclave signature.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_EPC,
        "Out of EPC memory.",
        NULL
    },
    {
        SGX_ERROR_NO_DEVICE,
        "Invalid SGX device.",
        "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
    },
    {
        SGX_ERROR_MEMORY_MAP_CONFLICT,
        "Memory map conflicted.",
        NULL
    },
    {
        SGX_ERROR_INVALID_METADATA,
        "Invalid enclave metadata.",
        NULL
    },
    {
        SGX_ERROR_DEVICE_BUSY,
        "SGX device was busy.",
        NULL
    },
    {
        SGX_ERROR_INVALID_VERSION,
        "Enclave version was invalid.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ATTRIBUTE,
        "Enclave was not authorized.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_FILE_ACCESS,
        "Can't open enclave file.",
        NULL
    },
    {
        SGX_ERROR_MEMORY_MAP_FAILURE,
        "Failed to reserve memory for the enclave.",
        NULL
    },
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret)
{
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist/sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++) {
        if(ret == sgx_errlist[idx].err) {
            if(NULL != sgx_errlist[idx].sug)
                printf("Info: %s\n", sgx_errlist[idx].sug);
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }
    
    if (idx == ttl)
    	printf("Error code is 0x%X. Please refer to the \"Intel SGX SDK Developer Reference\" for more details.\n", ret);
}

/* Initialize the enclave:
 *   Call sgx_create_enclave to initialize an enclave instance
 */
int initialize_enclave(void)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    
    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }

    return 0;
}

/* OCall functions */

// 记录某段代码的执行时间并保存
template<typename F>
void time_and_run(const string& name, F func) {
    auto t0 = chrono::high_resolution_clock::now();
    func();
    auto t1 = chrono::high_resolution_clock::now();
    long long us = chrono::duration_cast<chrono::microseconds>(t1 - t0).count();
    {
        lock_guard<mutex> lk(timing_mutex);
        timings.push_back({name, us});
    }
}

// 多线程预处理（leader单独，数据库实例并行）
void threaded_preprocessing() {
    // leader 预处理
    time_and_run("leader_preproc", [&](){
        ld.preprocessing(N[0], M, b, eta, L);
    });

    // control + targeted databases 并行
    vector<thread> ths;
    for (int i = 0; i < M-1; i++) {
        for (int j = 0; j < N[0]; j++) {
            ths.emplace_back([i,j](){
                string nm1 = "ctrl_DB_" + to_string(i) + "_" + to_string(j) + "_preproc";
                time_and_run(nm1, [&](){
                    control_databases[i][j].preprocessing(L, b, eta);
                });

                string nm2 = "tgt_DB_" + to_string(i) + "_" + to_string(j) + "_preproc";
                time_and_run(nm2, [&](){
                    targeted_databases[i][j].preprocessing(L, b, eta);
                });
            });
        }
    }
    for (auto &t : ths) t.join();
}

// 随机数生成阶段
void create_randomness(std::vector<std::vector<boost::asio::ip::tcp::socket>>& tee_sockets_to_cb,
    std::vector<std::vector<boost::asio::ip::tcp::socket>>& tee_sockets_to_tb) {
    //ecall_test(global_eid);
    int cols = N[0];
    size_t loc_len = size_t(M - 1) * cols * b;
    size_t rel_len = size_t(M - 1) * cols * b;
    size_t glo_len = size_t(M - 1) * cols;
	
    // 预分配扁平缓冲
    std::vector<int> loc_buf(loc_len);
    std::vector<int> rel_buf(rel_len);
    std::vector<int> glo_buf(glo_len);

    // 调用 Enclave
    ecall_create_randomness_flat(
        global_eid,
        M,
        N.data(),
        L,
        b,
        (M-1)*N[0]*b,
        (M-1)*N[0],
        loc_buf.data(),
        rel_buf.data(),
        glo_buf.data()
    );
	cout << "got out !!" << endl;
    // **在 host 端进行“切片”分发**，示例：
    // location_randomness for party i, column j:
    //   base = ((i*cols + j) * b)
    // relatived_randomness for party i, column j:
    //   base = ((i*cols + j) * b)
    send_randomness_though_net(tee_sockets_to_cb,tee_sockets_to_tb,loc_buf,rel_buf,glo_buf,M - 1,N[0],b);
}

// 多线程回复阶段
void threaded_reply(std::vector<std::vector<boost::asio::ip::tcp::socket>>& cb_sockets_for_leader, std::vector<std::vector<boost::asio::ip::tcp::socket>>& cb_sockets_for_tee, std::vector<std::vector<boost::asio::ip::tcp::socket>>& tb_sockets_for_leader, std::vector<std::vector<boost::asio::ip::tcp::socket>>& tb_sockets_for_tee) {
    vector<thread> ths;
    for (int i = 0; i < M-1; i++) {
        for (int j = 0; j < N[0]; j++) {
            ths.emplace_back([&,i,j](){
                // control db reply
                
                string nm1 = "ctrl_DB_" + to_string(i) + "_" + to_string(j) + "_reply";
                time_and_run(nm1, [&](){
                    control_databases[i][j].recv_randomness_though_net(cb_sockets_for_tee[i][j],b);
                    control_databases[i][j].recv_query_though_net(cb_sockets_for_leader[i][j]);
                    control_databases[i][j].create_and_send_reply(L, b, N[0]);
                    control_databases[i][j].send_answer_to_leader(cb_sockets_for_leader[i][j]);
                });
                // targeted db reply
                string nm2 = "tgt_DB_" + to_string(i) + "_" + to_string(j) + "_reply";
                time_and_run(nm2, [&](){
                    targeted_databases[i][j].recv_randomness_though_net(tb_sockets_for_tee[i][j],b);
                    targeted_databases[i][j].recv_query_though_net(tb_sockets_for_leader[i][j]);
                    targeted_databases[i][j].create_and_send_reply(L, b, N[0]);
                    targeted_databases[i][j].send_answer_to_leader(tb_sockets_for_leader[i][j]);
                });
            });
        }
    }
    for (auto &t : ths) t.join();
}

void ocall_print_string(const char *str)
{
    /* Proxy/Bridge will check the length and null-terminate 
     * the input string to prevent buffer overflow. 
     */
    printf("%s", str);
}


/* Application entry */
int SGX_CDECL main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);


    /* Initialize the enclave */
    if(initialize_enclave() < 0){
        printf("Enter a character before exit ...\n");
        getchar();
        return -1; 
    }
 
    /* Utilize edger8r attributes */
    edger8r_array_attributes();
    edger8r_pointer_attributes();
    edger8r_type_attributes();
    edger8r_function_attributes();
    
    /* Utilize trusted libraries */
    ecall_libc_functions();
    ecall_libcxx_functions();
    ecall_thread_functions();
    /* ci chu bian xie dai ma */
    // ecall_generate_and_distribute(global_eid);
	// 时间戳
    time_t now = time(nullptr);
    tm* lt = localtime(&now);
    outFile << "-------------------------\n";
    outFile << "运行时间：" << put_time(lt, "%Y-%m-%d %H:%M:%S") << "\n";

    // 参数解析
    parse_args(argc, argv, M, fileNames, N, K);
    L         = select_L(M);
    leader_id = M-1;
    sp_id     = M-2;
    eta       = (K + N[0] - 1) / N[0];
    b         = (3*eta + 1) / 2;

    // 初始化
    clients = vector<client>(M-1);
    control_databases  = vector<vector<database>>(M-1, vector<database>(N[0]));
    targeted_databases = vector<vector<database>>(M-1, vector<database>(N[0]));
    ld = leader(fileNames[leader_id], leader_id, M, N[0], b);
    chan = make_shared<channel>();

    outFile << "参与方数量：" << M << "\n";
    outFile << "全集大小：" << K << "\n";
    cout << "number of participants: " << M << endl;
    cout << "universe size: " << K << endl;

    for (int i = 0; i < M-1; i++) {
        for (int j = 0; j < N[0]; j++) {
            control_databases[i][j]  = database(fileNames[i], i, j, "base");
            targeted_databases[i][j] = database(fileNames[i], i, j, "not base");
        }
    }
    
    //threaded_preprocessing();
    boost::asio::io_context ioc;
    std::vector<std::vector<boost::asio::ip::tcp::socket>> leader_sockets_to_cb;
    std::vector<std::vector<boost::asio::ip::tcp::socket>> leader_sockets_to_tb;
    std::vector<std::vector<boost::asio::ip::tcp::socket>> tee_sockets_to_cb;
    std::vector<std::vector<boost::asio::ip::tcp::socket>> tee_sockets_to_tb;
    std::vector<std::vector<boost::asio::ip::tcp::socket>> cb_sockets_for_leader;
    std::vector<std::vector<boost::asio::ip::tcp::socket>> cb_sockets_for_tee;
    std::vector<std::vector<boost::asio::ip::tcp::socket>> tb_sockets_for_leader;
    std::vector<std::vector<boost::asio::ip::tcp::socket>> tb_sockets_for_tee;

    create_sockets(ioc, M - 1, N[0],leader_sockets_to_cb, leader_sockets_to_tb, tee_sockets_to_cb, tee_sockets_to_tb,cb_sockets_for_leader, cb_sockets_for_tee, tb_sockets_for_leader, tb_sockets_for_tee);
    cout << "sockets created!" << endl;

    auto t_start = chrono::high_resolution_clock::now();
    
    // 预处理阶段
    time_and_run("setup_phase", [](){threaded_preprocessing(); });
    cout << "preprocessing done!" << endl;

    // 查询阶段（仅leader）
    time_and_run("query_phase", [&](){
        ld.send_query();
    });

    
    cout << "query creation done!" << endl;
    // 传查询
    time_and_run("dispatch_query", [&](){
        ld.send_query_though_net(leader_sockets_to_cb,leader_sockets_to_tb,M - 1,N[0]);
    });
    cout << "query dispatch done!" << endl;
    // 随机数生成（单线程）
    time_and_run("randomness_gen", [&](){
        create_randomness(tee_sockets_to_cb,tee_sockets_to_tb);
    });
    cout << "randomness generation done!" << endl;
    // 回复阶段（并行数据库）
    threaded_reply(cb_sockets_for_leader, cb_sockets_for_tee,  tb_sockets_for_leader,  tb_sockets_for_tee);
    cout << "reply done!" << endl;
    // 计算阶段（leader）
    vector<int> intersection;
    time_and_run("compute_intersection", [&](){
        ld.recv_answer_from_databases(leader_sockets_to_cb,leader_sockets_to_tb,M - 1,N[0]);
        intersection = ld.calculate_intersection(M, N[0], b, L);
    });

    // 输出各阶段耗时
    {
        lock_guard<mutex> lk(file_mutex);
        for (auto &tg : timings) {
            outFile << tg.name << " 用时: " << tg.microseconds << " μs\n";
        }
        outFile << "交集大小: " << intersection.size() << "\n";
    }

    // cout << "交集大小：" << intersection.size() << endl;
    // cout << "intersection is:";
    // for (auto &e : intersection) {
    //     cout << " " << e;
    // }
    // cout << endl;
    auto t_end = chrono::high_resolution_clock::now();
    long long total_us = chrono::duration_cast<chrono::microseconds>(t_end - t_start).count();
    cout << "total time: " << total_us << " us" << endl;
    outFile << "总耗时: " << total_us << " 微秒" << endl;
    outFile << "总通信开销: " << com_bit/(1024.0*1024.0) << "MB" << endl;
    cleanup_all_sockets(
    leader_sockets_to_cb,
    leader_sockets_to_tb,
    tee_sockets_to_cb,
    tee_sockets_to_tb,
    cb_sockets_for_leader,
    cb_sockets_for_tee,
    tb_sockets_for_leader,
    tb_sockets_for_tee);
    /* Destroy the enclave */
    sgx_destroy_enclave(global_eid);
    
    printf("Info: SampleEnclave successfully returned.\n");
    
    // printf("Enter a character before exit ...\n");
    // getchar();
    return 0;
}

