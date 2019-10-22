# Homework 1
Use pthread to do parallel computation of PI using Monte Carlo Simulation

### Complie
```bash
# use makefile to compile pi_parallel to pi
make
# compile pi_serial
make serial
# clean pi
make clean
```

### Execute
```bash
./pi [num_cpu] [num_tosses]
./pi_serial [num_cpu(useless)] [num_tosses]
```

### Benchamrk
`num_tosses` = 100000000

|type       | core 1 | core 2 | core 4 |
|-----------|--------|--------|--------|
|pi_serial  |0.265s  |0.260s  |0.262s  |
|pi_parallel|0.265s  |0.146s  |0.091s  |