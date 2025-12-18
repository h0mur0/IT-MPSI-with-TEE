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

#include "Enclave.h"
#include "Enclave_t.h" /* print_string */
#include "sgx_trts.h"
#include <stdarg.h>
#include <stdio.h> /* vsnprintf */
#include <string.h>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>
#define NUM_B 10

/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
 
int printf(const char* fmt, ...)
{
    char buf[BUFSIZ] = { '\0' };
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
    return (int)strnlen(buf, BUFSIZ - 1) + 1;
}

void ecall_test() {
	printf("hello world");
}
sgx_status_t get_random_mod(int &out, int L) {
    int raw;
    sgx_status_t ret = sgx_read_rand(reinterpret_cast<unsigned char*>(&raw), sizeof(raw));
    if (ret != SGX_SUCCESS){ 
    return ret;}
    out = raw % L;
    return SGX_SUCCESS;
}

// 扁平化地生成 K 个 [0,L-1] 的随机数
sgx_status_t fill_random(int* out, int K, int L) {
    for (int i = 0; i < K; i++) {
        int v;
        sgx_status_t ret = get_random_mod(v, L);
        v = abs(v);
        if (ret != SGX_SUCCESS) return ret;
        out[i] = static_cast<int>(v);
    }
    return SGX_SUCCESS;
}
void ecall_create_randomness_flat(
    int M,
    int* N,
    int L,
    int b,
    int size_of_lr,
    int size_of_g,
    int* location_randomness,
    int* relatived_randomness,
    int* global_randomness
) {
    int cols = N[0];
    int loc_len      = (M - 1) * cols * b;
    int rel_len_pre  = (M - 2) * cols * b;
    int rel_len_total= (M - 1) * cols * b;
    int glo_len      = (M - 1) * cols;

    sgx_status_t ret1;
    // 1) 扁平化生成 location_randomness
    ret1 = fill_random(location_randomness, loc_len, L);
    if (ret1 != SGX_SUCCESS) printf("err1");

    // 2) 前 M-1 方的相关随机数
    ret1 = fill_random(relatived_randomness, rel_len_pre, L);
    if (ret1 != SGX_SUCCESS) printf("err2");

    // 3) 计算 special 方的相关随机数
    for (int j = 0; j < cols; j++) {
        for (int k = 0; k < b; k++) {
            int sum = (int)L - (int)(M - 1);
            for (int i = 0; i < M - 2; i++) {
                int idx = (i * cols + j) * b + k;
                sum += relatived_randomness[idx];
            }
            int idx_last = rel_len_pre + (j * b + k);
            relatived_randomness[idx_last] = sum;
        }
    }

    // 4) 全局随机数：生成一个 c
    int c32;
    ret1 = get_random_mod(c32, L - 1);
    if (ret1 != SGX_SUCCESS) printf("err3");
    int c = static_cast<int>(c32) + 1;  // 确保在 [1, L-1]
    for (int i = 0; i < glo_len; i++) {
        global_randomness[i] = c;
    }

    printf("suc1");
}

void ecall_security_channel(int* send_vec, int K, int* recv_vec) {
    memcpy(recv_vec, send_vec, K * sizeof(int));
}

