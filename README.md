# Name

Implementation of the Channel-aware Routing Protocol in Network Simulator for Underwater Acoustic Communication Networking

# Description
This codebase represents the development of Channel-aware Routing Protocol (CARP) on Aqua-Sim-NG which is a cross-layer routing protocol paradigm that utilizes the link estimation among nodes to determine the relay nodes to be used for packet forwarding.

# Requirements
1. NS-3.26, NS-3.24
2. Aqua-Sim-NG module
3. Ubuntu Operating System

# Installation
The following carefully enumerate the set of instructions required to setup the CARP module in Aqua-Sim-NG as found below:

# Usage
The usage of the protocol is similar to existing routing modules in AquaSim where an object of the AquaSimHelper is used to install the routing module on all the nodes in the underwater environment topology.


```bash

1. Download ns-3.27 via https://www.nsnam.org/release/ns-allinone-3.27.tar.bz2
2. Do a git clone of the Aqua-Sim-NG repo - https://github.com/rmartin5/aqua-sim-ng.git and place it in the ns-allinone-3.27/ns-3.27/src folder 
3. Return to the ./ns-allinone-3.27/ns-3.27/ and run the *./waf * command for the build process
4. Navigate to the repo of this project - https://github.com/sojiadvanced/aqua-sim-carp.git and clone the repo on your home directory
5. Copy out the 4 project files which comprise two header and two main files and paste it in the ns-allinone-3.27/ns-3.27/src/aqua-sim-ng/model folder
6. Once the above is done, run a build process *./waf *
```


```bash
AquaSimHelper asHelper; //Declares an object of the AquaSimHelper
asHelper.SetRouting("AquaSimCarp"); //Installs the CARP routing protocol module on the nodes
asHelper.SetRouting("AquaSimCarp",
		   "wait_time", MilliSeconds(5.0),
		   "hello_time", Seconds(1.0)); // This also initializes some of the variables with the AquaSimCarp module
```

# Support

You can reach out to the author of this project in case any form of assistance is required with the use of CARP in Aqua-Sim-NG. Contact details are provided below:

Adesoji Bello: bello@mtu.edu | sojiadvanced@gmail.com

# Roadmap

#Contributing

This project is very much opened for contribution to ensure gradual improvements of the active research on routing in the underwater wireless sensor networks to enhance the performance of the system.

# Authors & acknowledgment

Adesoji Bello:  bello@mtu.edu
Dr. Zhaohui Wang: Associate Professor at Michigan Technological University
Dmitrii Dugaev: PhD Researcher at City University of New York

# Licence
Copyright (c) 2020 UWSN Lab at the Michigan Technological University. All rights reserved.

