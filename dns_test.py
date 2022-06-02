import random
import string
import sys
import multiprocessing as mp
from time import sleep
from collections import Counter
from dns import *


def domain_test(dns_server, domain):
    rsvr = resolver.Resolver()
    rsvr.nameservers = [dns_server]

    try:
        ans = rsvr.resolve(domain)
    except Exception as e:
        res = (domain, type(e).__name__, [])
    else:
        ip_list = []
        for rdata in ans:
            ip_list.append(rdata.to_text())
        res = (domain, 'OK', ip_list)
    
    return res


def random_domain_test(dns_server):
    random_q = ''.join(random.choices(string.ascii_lowercase, k=5)) + '.com'
    return domain_test(dns_server, random_q)


def print_counter(counter):
    for k, v in counter.items():
        print(f'\t{k}: {v}')


if __name__ == '__main__':
    random.seed(42)
    
    LOCALHOST = '127.0.0.1'
    LOCAL_DNS = '192.168.2.1'

    a = sys.argv[1]
    if a == 'l':
        srv = LOCALHOST
    elif a == 'd':
        srv = LOCAL_DNS
    else:
        srv = a
    print('Testing DNS server:', srv)
    print()

    print('----------Basic Test----------')
    n = 10
    l = []
    for i in range(n):
        res = random_domain_test(srv)
        l.append(res)
    print(*l, sep='\n')
    st_l = [x[1] for x in l]
    print('SUMMARY:', f'TOTAL {n}')
    print_counter(Counter(st_l))
    print('------------------------------')
    print()

    with mp.Pool(processes=100) as pool:
        print('----------50x Test----------')
        n = 50
        l = pool.map(random_domain_test, [srv]*n)
        print(*l, sep='\n')
        st_l = [x[1] for x in l]
        print('SUMMARY:', f'TOTAL {n}')
        print_counter(Counter(st_l))
        print('------------------------------')
        print()

        print('----------200x Test----------')
        n = 200
        l = pool.map(random_domain_test, [srv]*n)
        # print(*l, sep='\n')
        st_l = [x[1] for x in l]
        print('SUMMARY:', f'TOTAL {n}')
        print_counter(Counter(st_l))
        print('------------------------------')
        print()

        print('----------500x Test----------')
        n = 500
        l = pool.map(random_domain_test, [srv]*n)
        # print(*l, sep='\n')
        st_l = [x[1] for x in l]
        print('SUMMARY:', f'TOTAL {n}')
        print_counter(Counter(st_l))
        print('------------------------------')
        print()

        print('----------1000x Test, sleep 1s per 100x----------')
        n = 100
        for i in range(10):
            l += pool.map(random_domain_test, [srv]*n)
            sleep(1)
        # print(*l, sep='\n')
        st_l = [x[1] for x in l]
        print('SUMMARY:', f'TOTAL {n*10}')
        print_counter(Counter(st_l))
        print('------------------------------')
        print()
