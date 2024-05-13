# You Spin Me Round Robin

You Spin Me Round Robin is a implementation for round robin scheduling
for a given workload and quantum length. It is implemented using a
doubly linked list.

## Building

```shell
$ make
```

## Running

cmd for running TODO
```shell
$ ./rr <file_path> <quantum_length>
```

results TODO
```shell
$ ./rr processes.txt 3
Average waiting time: 7.00
Average response time: 2.75

$ python -m unittest
Ran 2 tests in 0.467s
OK

```

## Cleaning up

```shell
make clean
```
