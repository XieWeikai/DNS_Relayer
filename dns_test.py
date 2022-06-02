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


def get_random_domain_list(n):
    l = [''.join(random.choices(string.ascii_lowercase, k=5)) + '.com'
            for _ in range(n)]
    return l


def print_counter(counter):
    l = list(counter.items())
    l.sort()
    for k, v in l:
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
    l = map(domain_test, [srv]*n, get_random_domain_list(n))
    print(*l, sep='\n') 
    st_l = [x[1] for x in l]
    print('SUMMARY:', f'TOTAL {n}')
    print_counter(Counter(st_l))
    print('------------------------------')
    print()
    sleep(10)

    with mp.Pool(processes=100) as pool:
        print('----------50x Test----------')
        n = 50
        l = pool.starmap(domain_test, zip([srv]*n, get_random_domain_list(n)))
        print(*l, sep='\n') 
        st_l = [x[1] for x in l]
        print('SUMMARY:', f'TOTAL {n}')
        print_counter(Counter(st_l))
        print('------------------------------')
        print()
        sleep(10)

        print('----------200x Test----------')
        n = 200
        l = pool.starmap(domain_test, zip([srv]*n, get_random_domain_list(n)))
        # print(*l, sep='\n') 
        st_l = [x[1] for x in l]
        print('SUMMARY:', f'TOTAL {n}')
        print_counter(Counter(st_l))
        print('------------------------------')
        print()
        sleep(10)

        print('----------500x Test----------')
        n = 500
        l = pool.starmap(domain_test, zip([srv]*n, get_random_domain_list(n)))
        # print(*l, sep='\n') 
        st_l = [x[1] for x in l]
        print('SUMMARY:', f'TOTAL {n}')
        print_counter(Counter(st_l))
        print('------------------------------')
        print()
        sleep(10)

        print('----------1000x Test, sleep 1s per 50x----------')
        n = 50
        for i in range(20):
            l += pool.starmap(domain_test,
                    zip([srv]*n, get_random_domain_list(n)))
            sleep(1)
        # print(*l, sep='\n') 
        st_l = [x[1] for x in l]
        print('SUMMARY:', f'TOTAL {n*20}')
        print_counter(Counter(st_l))
        print('------------------------------')
        print()
