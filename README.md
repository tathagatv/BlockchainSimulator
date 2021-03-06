
# Simulation of a P2P Cryptocurrency Network

## Guide: Prof. Vinay Ribeiro, CS 765, IIT Bombay 

### Requirements
- Simulation
    - Make `(Optional) (See below)`
    - C++17 `Tested on GNU C++ (Ubuntu 20.04) and MinGW (Windows)`
- Visualization and Statistics
    - Python 3 `Recommended Python >= 3.6`
    - graphviz `sudo apt install graphviz`
    - Pip packages `python3 -m pip install -r requirements.txt` 

### Compilation
To compile the simulator binary, simply run make. See Makefile for more information.
```
make
```
If you do not have make installed or if you're not working in linux, you can use the following command to compile the final binary, which can be used readily.
```
g++ -g -Wall -std=c++17 -O2 src/simulator.cpp src/transaction.cpp src/peer.cpp src/blockchain.cpp src/link.cpp src/block.cpp src/event.cpp src/attacker.cpp src/main.cpp -o blockchain_simulator
```

### Running
The simulator binary is named `blockchain_simulator` (with `.exe` extension, if used in Windows), and uses the following parameters.
```
Usage: ./blockchain_simulator [options] 

Optional arguments:
-h --help                       shows help message and exits
-v --version                    prints version information and exits
-n --peers                      Number of peers in the network [default: 40]
-e --edges                      Number of edges in the peer network [default: 150]
-z --slowpeers                  Fraction of slow peers in the network [default: 0.4]
-t --time_limit                 Run the simulation upto a time limit (in seconds) [default: 1.79769e+308]
-Ttx --txn_interarrival         Mean of exponential distribution of interarrival time between transactions [default: 40]
-Tk --mining_time               Mean of exponential distribution of time to mine a block [default: 1000]
-s --seed                       Seed for random number generator [default: 42]
-txn --max_txns                 Run simulation till max transactions are generated, 0 indicates infinity [default: 0]
-blk --max_blocks               Run simulation till max blocks are generated, 0 indicates infinity [default: 100]
-v --verbose                    Print output log [default: false]
-it --invalid_txn_prob          Probability of generating an invalid transaction [default: 0.05]        
-ib --invalid_block_prob        Probability of generating an invalid block [default: 0.05]
-zeta --attacker_connection     Fraction of honest nodes adversary is connected to [default: 0.5]       
-a --alpha                      Fraction of hash power belonging to attacker [default: 0.35]
-adv --adversary_type           Type of adversary, choose from (none, selfish, stubborn) [default: "none"]
```
Kindly note all the parameters have default values, so the executable will NOT report an error in case of absence of a parameter.

**NOTE**: Verbose option is known to change the state of pseudo-random number generator (mt19337), therefore, the outputs might differ when run with the exact same parameter while switching the verbose flag.

### Cleanup
To remove all the intermediate files and the binary, you can run
```
make clean
```
or just the delete the `blockchain_simulator` executable.

### Visualization
The simulator outputs the following files:
- `output/block_arrivals/*.txt` each txt file contains the arrival timestamps of each block at each peer, sorted by the block ID.
- `output/termination_blockchains/*.txt` contains the edges of the blockchain formed at the termination point of the simulation (these blockchains are likely to be different at each peer, depending on the parameters set).
- `output/final_blockchains/*.txt` contains the edges of the blockchain formed at the end of the simulation, when stability is reached (these blockchains will be same at all peers, except the attacker/adversary).
- `output/peer_stats/*.txt` each txt file contains the peer attributes (as per the final blockchain at that peer) relevant for post-simulation analysis.

`src/visualize.py` draws the blockchain tree by reading in the edgelist and saves it into the file `output/blockchain_image.png` along with printing appropriate statistics, and a plot containing the fractions of hash powers and block contributions for each peer saved in `output/block_stats.txt`.

You can simply execute the python script to automatically perform the aforementioned tasks.
```
python3 src/visualize.py
```

### Internals
- The simulator has three termination conditions, and will terminate as soon as any of the three is satisfied.
    - `max_blocks`: Run till a specific amount of blocks are mined.
    - `max_txns`: Run till a specific amount of transactions are generated.
    - `time_limit`: Run till the virtual timestamp is reached.
- The simulator processes events **beyond** the termination conditions as well. Since each peer can have a different blockchain at any point of time, the simulations runs till equilibrium is reached. Beyond the termination point, any event which generates transactions or blocks will not be processed, thereby allowing all the pre-existing ones to flow through the network and fully updating the blockchain state of each peer. After equilibirum is reached, the blockchain at all the peers is same and is exported for further static analysis.
