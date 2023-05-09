import matplotlib.pyplot as plt
import numpy as np

if __name__ == '__main__':
    server_num = np.arange(1, 11)
    time1 = np.array([53, 28, 20, 18, 16, 15, 15, 15, 15, 15])
    customer_num = np.array([5, 10, 15, 20, 25, 30, 35, 40, 45, 50])
    time2 = np.array([16, 19, 26, 30, 34, 39, 41, 55, 56, 58])

    plt.figure()
    plt.plot(server_num, time1, label='customer num = 10', marker='o')
    plt.xlabel('server num')
    plt.ylabel('time')
    plt.legend()
    plt.savefig('time1.png')

    plt.figure()
    plt.plot(customer_num, time2, label='server num = 5', marker='o')
    plt.xlabel('customer num')
    plt.ylabel('time')
    plt.legend()
    plt.savefig('time2.png')