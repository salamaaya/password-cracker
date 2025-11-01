# Project Title

Password Cracker - CS810

## Description

A dictionary-attack password cracker designed for GPU experimentation — parallelizable kernels for high-throughput cracking and easy benchmarking.

### Dependencies

Visit [rockyou](https://weakpass.com/wordlists/rockyou.txt) to install a dataset of most commonly used passwords into the project's root.
Alternatively, can use any dataset of your choosing as long as it's formatted as a single password per line.

Once you've installed the rockyou dataset, run the following in the project's root:

```
gunzip rockyou.txt.gz
```

### Usage

```
./dictioanry <password> [maximum length]
```

To create an executable, run:
```
make
```

Examples:
```
./dictionary password1234 28
./dictionary strongpassword
```

Note: If the maximum is not set, the default will be set to 128
