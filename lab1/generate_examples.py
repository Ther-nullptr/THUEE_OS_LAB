import argparse
from random import randint

if __name__ == '__main__':
    argparser = argparse.ArgumentParser()
    argparser.add_argument('--ncustomer', required=False, default=10, type=int, help='Number of customers')
    argparser.add_argument('--minservtime', required=False, default=1, type=int)
    argparser.add_argument('--maxservtime', required=False, default=10, type=int)
    argparser.add_argument('--minentertime', required=False, default=1, type=int)
    argparser.add_argument('--maxentertime', required=False, default=10, type=int)
    args = argparser.parse_args()
    n_customer = args.ncustomer
    min_serv_time = args.minservtime
    max_serv_time = args.maxservtime
    min_enter_time = args.minentertime
    max_enter_time = args.maxentertime
    enter_time_list = []
    serv_time_list = []
    for i in range(n_customer):
        enter_time_list.append(randint(min_enter_time, max_enter_time))
        serv_time_list.append(randint(min_serv_time, max_serv_time))
    enter_time_list.sort()
    lines = []
    for i in range(n_customer):
        lines.append(f'{i} {enter_time_list[i]} {serv_time_list[i]}\n')
    with open('test.txt', 'w', encoding='utf-8') as f:
        f.writelines(lines)
