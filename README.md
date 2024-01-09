# TCP_Protocol

By the collaboration of [Elijah Ye](https://elijah-ye.github.io) and [Ananya Kommalapati](https://github.com/MeanPaper) <br>
UIUC CS438 FA23 | September 2023 ~ October 2023

We used `UDP` to implement our own version of `TCP`. We are able to tolerate packet drops, allow other concurrent connections a fair chance, and it is not overly nice to other connections (should not give up the entire bandwidth to other connections).

## Running the Program

In order to run the program, you need to have two VM instances. In our case, we used VirtualBox and fired up two Linux VM instances on our local machine. For more set up information, please read [here](/docs/set_up.pdf). For more details about the project, please read [here](/docs/tcp_protocol_project.pdf).

### Client

```
$ make
$ ./reliable_receiver
```

### Server
```
$ make
$ ./reliable_sender [hostname] [port] [filename] [bytesToTransfer]
```


### To run program with customized speed and drop rate

If your network interface inside the VM is `eth0`, then
run (from inside the VM) the following command:

```
$ sudo tc qdisc del dev eth0 root 2>/dev/null
```

to delete existing tc rules. Then use,
```
$ sudo tc qdisc add dev eth0 root handle 1:0 netem delay 20ms loss 5%
```
followed by
```
$ sudo tc qdisc add dev eth0 parent 1:1 handle 10: tbf rate 20Mbit burst 10mb latency 1ms
```

will give you a 20Mbit, 20ms RTT link where every packet sent has a 5% chance to get dropped.
Simply omit the `loss n%` part to get a channel without artifcial drops.
(You can run these commands just on the sender; running them on the receiver as well won’t
make much of a difference, although you’ll get a 20ms RTT if you don’t adjust the delay to
account for the fact that it gets applied twice.)

<br>

## Academic Integrity

Please review the University of Illinois Student Code,
particularly all subsections of [Article 1, Part 4 - Academic Integrity Policy and Procedure](https://studentcode.illinois.edu/article1/part4/1-401).



## Legal Notice

This work is protected under the [GNU General Public License v3.0](https://www.gnu.org/licenses/gpl-3.0.en.html). <br>
**Copyright (c) 2024 Elijah Gaohan Ye, Ananya Kommalapati, Romit Roy Choudhury**

    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose, without fee, and without written agreement
    is hereby granted, provided that the following two paragraphs appear
    in all copies of this software.

    IN NO EVENT SHALL THE AUTHOR OR THE UNIVERSITY OF ILLINOIS BE LIABLE TO
    ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
    DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
    EVEN IF THE AUTHOR AND/OR THE UNIVERSITY OF ILLINOIS HAS BEEN ADVISED
    OF THE POSSIBILITY OF SUCH DAMAGE.

    THE AUTHOR AND THE UNIVERSITY OF ILLINOIS SPECIFICALLY DISCLAIM ANY
    WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE
    PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND NEITHER THE AUTHOR NOR
    THE UNIVERSITY OF ILLINOIS HAS ANY OBLIGATION TO PROVIDE MAINTENANCE,
    SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

> The software programs described in the documents are confidential and proprietary
> products of Synopsys Corp. or its licensors. The terms and conditions governing
> the sale and licensing of Synopsys products are set forth in written agreements
> between Synopsys Corp. and its customers. No representation or other affirmation
> of fact contained in this publication shall be deemed to be a warranty or give rise
> to any liability of Synopsys Corp. whatsoever. Images of software programs in use
> are assumed to be copyright and may not be reproduced.

> The documents are for informational and instructional purposes only. The ECE 411 teaching
> staff reserves the right to make changes in specifications and other information contained
> in this publication without prior notice, and the reader should, in all cases, consult
> the teaching staff to determine whether any changes have been made.