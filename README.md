# Course Project: Operating System

## LAB1 Banker's Server Problem

Source code is in `lab1/` directory.

### build

```bash
g++ main.cpp -o main -lpthread
```

### run

To generate random data, run:

```bash
python generate_example.py
```

You can see the result in `test.txt`, like this:

```
0 1 7
1 2 1
2 3 7
3 4 1
4 4 2
5 6 2
6 6 5
7 7 9
8 9 6
9 9 3
```

then run the main program:

```bash
./main [num_of_servers]
```

you can see the result in `result.txt`.

## LAB4 Process Scheduling

### build

```bash
g++ main.cpp -o main
```

### run

prepare your test data in `test.txt`, like this:

```
140
1 1 0 30 15
2 1 0 40 15
3 1 0 50 5
4 0 135 165 5 
5 0 135 160 5
```

then run the main program:

```bash
./main [ RMS(1) | EDF(2) | LLF(3) ]
```

you can see the result in `result.txt`.

## LAB6 Pipe Driver

### build

```bash
make
sudo insmod mypipe.ko
sudo mknod /dev/mypipe c 200 0
sudo chmod 666 /dev/mypipe
```

### run

```bash
sudo echo 114514 > /dev/mypipe
sudo echo 114514 > /dev/mypipe
sudo echo 1919810 > /dev/mypipe
sudo cat /dev/mypipe 
```

### remove

```bash
sudo rmmod mypipe
sudo rm -f /dev/mypipe
```