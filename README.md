# Hawkeye NS-3 simulator

This is an NS-3 simulator for "RDMA Network Performance Anomalies Diagnosis with Hawkeye". We add Hawkeye components to [HPCC(SIGCOMM' 2019)'s NS-3 simulator](https://github.com/alibaba-edu/High-Precision-Congestion-Control) to simulate RDMA network performance anomalies diagnosis.

It is based on NS-3 version 3.17.

## Quick Start

### Build
`./waf configure`

Please note if gcc version > 5, compilation will fail due to some ns3 code style.  If this what you encounter, please use:

`CC='gcc-5' CXX='g++-5' ./waf configure`

### Experiment config
Please see `mix/config.txt` for example. 

`mix/config_doc.txt` is a explanation of the example (texts in {..} are explanations).

### Run
The direct command to run is:
`./waf --run 'scratch/third mix/config.txt'`

## Files added/edited based on HPCC's NS3 simulator
The major ones are listed here. There could be some files not listed here that are not important or not related to core logic.

`point-to-point/model/rdma-queue-pair.cc/h`: queue pair

`point-to-point/model/rdma-hw.cc/h`: the core logic of congestion control. An agent is added here to monitor RTTs and send polling packets.

`point-to-point/model/switch-node.cc/h`: the node class for switch. Some telemetry structures are maintained to support RDMA NPA diagnosis.

`point-to-point/model/switch-mmu.cc/h`: the mmu module of switch.

`network/utils/custom-header.cc/h`: a customized header class for speeding up header parsing. We add some new headers to support Hawkeye.

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
