import random
import string
import subprocess
import time

def generate_random_strings(n, length=50):
    return [''.join(random.choices(string.ascii_letters + string.digits, k=length)) for _ in range(n)]


def write_random_strings_to_files(K, t, m, members):
    data_files = {}
    for i in range(1, members):
        data_files[f"data{i}.txt"] = random.sample(range(K), m)
    data_files[f"data{members}.txt"] = random.sample(range(K), t)
    for filename, strings in data_files.items():
        with open(filename, 'w') as f:
            for data in strings:
                f.write(f"{data}\n")

def test_one_time(K, m, n):  
    other = m
    write_random_strings_to_files(K, m, other, n)
    command1 = [
        "./main_ubw21", "-m", str(n), "-t"
    ] + [f"data{i}.txt" for i in range(1, n+1)] + ["-n"] + ["5"] * n + ["-K", str(K)]
    # subprocess.run(command1)
    
    command2 = [
        "./main", "-m", str(n), "-t"
    ] + [f"data{i}.txt" for i in range(1, n+1)] + ["-n"] + ["5"] * n + ["-K", str(K)]
    # subprocess.run(command2)
    
    command3 = [
        "./app", "-m", str(n), "-t"
    ] + [f"data{i}.txt" for i in range(1, n+1)] + ["-n"] + ["5"] * n + ["-K", str(K)]
    subprocess.run(command3)

password = "1"

cmd = "tc qdisc del dev lo root"
        
# full_cmd = f"echo '{password}' | sudo -S {cmd}"
full_cmd = f"sudo {cmd}"
try:
    result = subprocess.run(
        full_cmd,
        shell=True,
        capture_output=True,
        text=True,
        check=True
    )
except:
    pass

cmd = "tc qdisc add dev lo root netem rate 20Gbit delay 0.01ms"
        
# full_cmd = f"echo '{password}' | sudo -S {cmd}"
full_cmd = f"sudo {cmd}"

result = subprocess.run(
    full_cmd,
    shell=True,
    capture_output=True,
    text=True,
    check=True
)
test_one_time(2**15,2**10,10)

for n in[10,25,40]:
    for m in [2**18,2**19,2**20]:
        for K in [2 ** 20,2**21,2**22]:
            test_one_time(K, m, n) 
            time.sleep(0)

cmd = "tc qdisc del dev lo root"
        
# full_cmd = f"echo '{password}' | sudo -S {cmd}"
full_cmd = f"sudo {cmd}"

result = subprocess.run(
    full_cmd,
    shell=True,
    capture_output=True,
    text=True,
    check=True
)

cmd = "tc qdisc add dev lo root netem rate 200Mbit delay 48ms"
        
        # 使用echo传递密码
# full_cmd = f"echo '{password}' | sudo -S {cmd}"
full_cmd = f"sudo {cmd}"

result = subprocess.run(
    full_cmd,
    shell=True,
    capture_output=True,
    text=True,
    check=True
)
test_one_time(2**15,2**10,10)

for n in[10,25,40]:
    for m in [2**18,2**19,2**20]:
        for K in [2 ** 20,2**21,2**22]:
            test_one_time(K, m, n) 
            time.sleep(0)


# full_cmd = f"echo '{password}' | sudo -S {cmd}"
full_cmd = f"sudo {cmd}"
try:
    result = subprocess.run(
        full_cmd,
        shell=True,
        capture_output=True,
        text=True,
        check=True
    )
except:
    pass