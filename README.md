# NDN Original Cache Poisoning Scenarios
Scenarios for NDN Bad Data Mitigation Project (Ran w/o any edits to the NFD or ndnSIM)

Note: Cloned from original NDN template scenario (https://github.com/named-data-ndnSIM/scenario-template.git) for NDN SIM v2.0 on Apr. 19th, 2017

Helpful Tips on Using the Template
==================================
To send your logs from the tracers to the results folder, specify the file to write the results to as "results/...". It will create the file and put it into the results folder. For more info on tracers, refer to: http://ndnsim.net/2.3/metric.html

Don't forget to follow the guidelines outlined in http://ndnsim.net/2.3/best-practices.html?highlight=ns_log

Refer to the original NDN template scenario on how to compile and run scenarios using the template (though don't re-git-clone ns-3 etc. just skip to the part of install ns3 since that part is required during the first time. After than you can add as many scenario templates into your ndnSIM folder to your hearts content and just run ./waf configure per template)

Things to Note when Looking At this Code
========================================
Due to Ack and Retransmissions, the Key requests had to consume a sequence number. And so when looking that the App Delay logs, the sequence numbers that don't really show up are consumed by the Key Request Interests

Currently most of the mechanisms for verification in this code are "hardcoded" in the sense that it is heavily dependent on name and data content size to perform verification. This was due to time constraints and limitations of the simulator. If you are to be based purely on name to verify the packet being good or bad, you would get a fatal error of name component out of bounds. But exclude needs the name component so both the data content check and name are being used here. Actually signing and verification will add extra overhead to the results possibly... (basically this is the ideal scenario where signature verification can be done in milliseconds) 

Most of the code is also based on existing code in ndnSIM so many of the lines look similar to that. Original code is property of the University of California Regents that's publicly distributed under a GNU license and so thank you to the original authors :)

Available simulations
=====================

Scenario: Simple Signer Example
-------------------------------

Key Points: Creating multiple Producers in a network, "Verification" of data

Creates a simple layout where you have a key producer, a producer, and a consumer lined up such that the last node connects to both the key producer and producer. The consumer requests for data from the producer and will then request the key from the key producer. Currently Security Toy Client App (the consumer for the scenario) is "hardcoded" to request for the key prefix once data from the producer comes in.

Scenario: Grid Signer Example 
------------------------------

Key Points: Creating multiple Producers in a Grid Network, "Verification" of data

Creates a grid where in one corner is the consumer, in another corner the producer, and in another, the key producer. Follows the same example as the simple signer just that this one is in a grid layout.

Scenario: Basic Cache Poisoning Scenario
-----------------------------------------

Key Points: Exclude filter, Custom Evil Producer

Introduces the Evil Producer into the network. This network is a linear network where the last neutral node is connected to the producer, the 2nd to last neutral node is connected to the key producer, and the node closest to the consumer is connected to the Evil Producer (who currently generates DATA packets with "/evil" appended at the end of the name). The consumer checks the data content size (since can't really simulate easily actual payload) and ends up in pursuit mode for the correct data. While in pursuit mode, the consumer will send out an interest with Exclude set. Currently Exclude only works for the name => why it's an added suffix to the name for the DATA packets Evil Producer creates

Scenario: Crowded Cache Poisoning Scenario
------------------------------------------

Key Points: Exclude filter, Custom Evil Producer, Multiple Consumers, Delay Start

Builds on top of the Basic Cache Poisoning Scenario by adding 2 more consumers into the network such that all 3 consumers are at the same spot. However, the added twist to this is that each consumer starts with a different delay start. This is to see the effects of cache poisoning on the other 2 consumers who will request the same packets as the third. (It's pretty bad if you take a look into the results log...)

Scenario: Distributed Cache Poisoning Scenario
----------------------------------------------

Key Points: Exclude filter, Custom Evil Producer, Multiple Consumers in a Grid, Delay Start

Takes the Crowded Cache Poisoning Scenario and puts it into a grid. Consumers are on the left side of the grid and the producers are on the right. Looks into how NDN reacts in such a situation where the consumers are not from the same starting point but have various hop distances and delays from evil, good, and signer producers.

