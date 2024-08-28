## Socket concepts:

What is a socket?
- channel for 2 parties to communicate over a network

Client-Server Model
- one initiating the channel is called a client, and the server is the one waiting for new clients
- a non-toy server can serve multiple clients simultaneously

Request-Response Model
- client sends 1 request and waits for 1 response
- server processes and sends the response for each request in a loop

TCP, UDP, Applications
- 2 types of channels, packet based and byte-stream based
UDP: packet based, a packet is a message of a certain size from the application's POV
TCP: provides a contiuous stream of bytes, has no boundaries within it

## TCP/ICP Review

Layers of Protocols

Network protools are divided into different layers, where the higher layer depends on the lower layer,
and each layer provides different capabilities.
```
 top
  /\    | App |     message or whatever
  ||    | TCP |     byte stream
  ||    | IP  |     packets
  ||    | ... |
bottom
```


The layer below the TCP is the IP layer, each IP packet is a message with 3 components
- senders address
- recievers address
- message data

TCP: Reliable Byte Streams
Communication with a packet-based scheeme is not easy, there are lots of problems for applications to solve
- what if message data exceeds capacity of a single packet?
- what if a packet is lost?
- out of order packets?
- managing the lifetime of connections

To make things simple, the next layer is added on top of IP packets, TCP provides:
- byte streams instead of packets
- reliable and ordered delivery
A byte stream is an ordered sequence of bytes, a protocal, rather than the application, is used to make sense of
these bytes, protocols are lie file formats, expect that the total length is unknown and the data is read in one pass
UDP is on same layer as TCP, but is pakcet based like the lower layer, UDP just adds port numbers over IP packets

## Socket Primitives:

Applications refer to sockets by Opaque OS handles, in the same way that twtr handles are used to refer to twtr uses
In linux, a handle iss called a file descriptor(fd), which is an integer that is unique to the process
The socket() syscall allocates and returns a socket fd, which is used later to create a communication channel
A handle must be closed when you're done to free the associate resources on the OS side.

Listening Socket and Connection Socket
A TCP serve rlistens on a particular address (IP + port) and accepts client connections from that address,
the listening address is also represented by a socket fd, when you accept a new client connection, you get the socket fd
of the TCP connection

2 types of socket handles:
- listening sockets - obtained by listening on an address
- connection sockets - obtained by accepting a client connection from a listening socket
bind() : configure the listneing address of a socket
listen() : make the socket a listening socket
accept() : return a client connection socket, when available

```
fd = socket()
bind(fd, address)
listen(fd)
while True:
    conn_fd = accept(fd)
    do_something(conn_fd)
    close(conn_fd)
```


Read and Write
- read/write : read/write with a single continuous buffer
- readv/writev : read/write with multiple buffers
- recv/send : extra flag
- recvfrom/sendto : also get/set the remote address (packet-based)
- recvmsg/sendmsg : readv/writev with more flags and controls
- recvmmsg/sendmmsg : muiltiple recvmsg/sendmmsg in 1 syscall

Connect From a Client
- connect() syscall is for initiating a TCP connection form the client side
```
fd = socket()
connect(fd, address)
do_something(fd)
close(fd)

```

the type of a socket is determined afte rhte listen() or connet() syscall

## Demystifying 'struct sockaddr'

```
struct sockaddr {
    unsigned short sa_family; //AD_INET, AD_INET6
    char sa_data[14]; // useless
}

struct sockaddr_in {
    short sin_family // AF_INET
    unsigned short sin_port // port number, big endian
    struct in_addr sin_addr; // IPv4 adderss
    char sin_zer[8] // useless
}

struct sockaddr_in6 {
    uint16_t        sin6_family;    // AF_INET6
    uint16_t        sin6_port;      // port number, big endian
    uint32_t        sin6_flowinfo;
    struct in6_addr sin6_addr;      // IPv6 address
    uint32_t        sin6_scope_id;
};

struct sockaddr_storage {
    sa_family_t     ss_family;      // AF_INET, AF_INET6
    // enough space for both IPv4 and IPv6
    char    __ss_pad1[_SS_PAD1SIZE];
    int64_t __ss_align;
    char    __ss_pad2[_SS_PAD2SIZE];
};
```

- struct sockaddr_storage is large enough for both IPv4 and IPv6, use for prod
- struct sockaddr_in and sockaddr_in6 are the concrete structres for IPv4 and IPv6
- cast a struct sockadd_storage refernce to struct sockadd_in or struct sockaddr_in6 to initialize and read the strucutre

## Syscalls, API's, and Libraries
- when invoking the syscaalls on linux, you are actually invoking a think wrapper in lib - a wrapper to the
stable linux syscall interface

- you can get the address (ip + port) for both local and peer
- getpeername() : get the remote peer's adderss (returned by accept())
- getsockname() : get my exacty address (after binding to a wildcard reference)

- bind() can be used on the client socket before connect() to speicify the source address
- without this, the OS will automatically select a source address, useful for selecting a particular interface
from multiple interfaces
- if the port in bind() is zero, the OS will automatically choose a port

## Chapter 4: Protocol Parsing

- our server will eb able to process multiple requests from a client, so we need to implement some sort of
protocl to split requests apart form the TCP byte stream, easiest way is to declare how long the request is at the beignning of the request
+-----+------+-----+------+--------
| len | msg1 | len | msg2 | more...
+-----+------+-----+------+--------

protocol consists of 2 parts: a 4 byte little-endian integer indicating the lenght of the request, and the ]
variable-length request

### Text vs Binary:
- text protocols have the advantage of beign human readable, disadvantage of complexity
- complex due to variable lenght strings, the text parsing code incolves a lot of lnegth calculations, boudn checks, and decisions
ex) parse an integer in decimal tet "1234", for every byte, you have to check for the end of the buffer and whether the
integer has ended, in contrast to a fixed-width binary integer

### Streaming Data:
- this protocol includes the len of the message at the begining, real worlds protocols often use less obvious ways to indicate the lenght
of the message eg) streaming data continuously without knowing the full length of the output (chunked transfer encoding HTTP)
- this encoding wraps incoming data into a message format that starts with the len of the message, client recives a stream
of messages, until a special message indicates the end of the stream

### Further Considerations:
- protocol parsing code requires at least 2 read() syscalls per request, can be reduced by using "buffered IO", that is
read as much as you can into a buffer at once, then try to parse multiple requests from that buffer


## Chapter 5: Event Loop and Nonblocking IO
- 3 ways to deal with concurrent connections in server-side network programming: forking, multithreading, and event loops
forking : create new processes for each client connection to achieve concurrrency
multithreading : uses threads instead of processes
event loop : uses polling and nonblocking IO and usually runs on a single thread, due to overhead of processes and threads, most modern
production grade software uses eventloops for networking

pseudocode for event loop of server:

```
all_fds = [...]
while True:
    active_fds = poll(all_fds)
    for each fd in active_fds:
        do_something_with(fd)

def do_something_with(fd):
    if fd is a listening socket:
        add_new_client(fd)
    elif fd is a client connection:
        while work_not_done(fd):
            do_something_to_client(fd)

def do_something_to_client(fd):
    if should_read_from(fd):
        data = read_until_EAGAIN(fd)
        process_incoming_data(data)
    while should_write_to(fd):
        write_until_EAGAIN(fd)
    if should_close(fd):
        destroy_client(fd)
```

- instead of just doing things with fds, we use the 'poll' operation to tell us which fd can be operated immediately without blocking
- when we perform an IO operation on an fd, the operation should be performed in nonblocking mode
- in blocking mode, read blocks the caller when there is no data in the kernel, write blocks when the write buffer is full, and accept blocks when
there are no new connections in the kernel queue, operations that fail with EAGAIN must be retried after the readiness was notified by the poll
- the poll is the sole blocking operation in an event loop, everything else must be nonblocking, thus a single thread can
handle multiple concurrent connections
- all blocking networking IO api's have a nonblcoking mode, api's that do not have a nonblocking mode should be performed in thread pools

syscall for setting an fd to nonblocking mode is fcntl:
```

// gets current flags for the file descriptor using fcntl
// flags |= O_NONBLOCK; we use a bitwise OR on the O_NONBLCOK flag agains the current flags, adds non-blocking propety
without affecting other flags
// then call fcntl again, with F_SETFL to set the new flags, cast to void to ignore return val of fcntl
static void fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno) {
        die("fcntl error");
    }
}
```

## Chapter 7: Basic Server: get, set, del

Protocol: the command in out design is a list of strings, like set key val, encode command with following scheme:

+------+-----+------+-----+------+-----+-----+------+
| nstr | len | str1 | len | str2 | ... | len | strn |
+------+-----+------+-----+------+-----+-----+------+

where nstr is num of strings, len is length of following string

response is a 32 bit status code followed by response string
+-----+---------+
| res | data... |
+-----+---------+


## Chapter 8: Hash Tables

### Considerations for Coding Hashtables
1) Worst Cases Matter Most
- a signel redis instance can scale to hundreds of GB of memory, while most uses of redis are not real-time applications,
latency becomes a problem at this scale
- hashtables are known for their o(1) amortized cost, but a single resize will be o(n), and can bring you down no matter how
fast it is on average
- a partial solution is sharding: dividing the key space into smaller instances, this also scales throughput, but complicates things with
new components
- real solution is to progressively move keys to the new table, so that an insert does a bounded amount of work and a lookup can use both
tables during resizing
- besides resizing, collisions are also a source of worst cases, balanced tree structures, although slower, are preffered for latency sensitive apps

2) CPU cache affects performance
- many benchmarks show a big performance advantage for open addressing because they often use integer keys that fit in the slots,
while a chaining hashtable uses heap-allocated linked lists
Key lookup of a single proble:
- open addressing: 1 memory read from the slot
- chaining: 2 memory reads, slot and heap object
Key lookup of multiple probes:
- open addressing: depending on strategy, key may be found near the initial slot
- chaining: following multiple list nodes
open addressing is more CPU-cache friendly, even if keys are also heap-allocated

### Considerations for Using Hashtables
1) Heap allocations per key
- open addressing with data stored in slots(integers) : 0 allocations
- open addressing with pointers to data(string) : 1 allocation
- chaining with data stored in teh list node: 1 allocation for the list node
- chianing with pointers to data: 2 allocations

chaining always seems to require 1 more allocation, however the case of chaining with pointers to data can be avoided by embedding the
list node into the key object, so that both chaining and open addressing require 1 allocation when keys are heap-allcoated
- embedding the list node is called instrusive data structures

2) Memory Overhead per key
in open addressing tables, the overhead comes from the slots:
- the slot contains auxillary data such as the deletion bit
- the slot contains the key, empty slots are proportional to used slots

in chianig tables using linked lists:
- each slot is a pointer of constant size
- each key needs a list node of constant siz e

comparison:
open adderssing with fixed size keys: proportional overhead
open addressing with pointers to keys: fixed overhead per key
chaining using linked lists: fixed overhead per key

### Instrusive Data Structures:
- a way of making generic collections,whcih means the code is usable for different types of data
ex) several ways to make a linked list node

C++ templates:
template <class T>
struct Node {
    T data;
    struct Node *next;
};

void * in C:
struct Node {
    void *data; // points to anything
    struct Node *next;
};

Macro hacks in C:
#define STR(s) #s
#define DEFINE_NODE(T) struct Node_ ## STR(T) { \
    T data; \
    struct Node_ ## STR(T) *next; \
}

Fixed sized byte array (not generic at all):
struct Node {
    char data[256];     // should be enough?
    struct Node *next;
};

### Embedding Structures Within Data: 
- all of the above methods are based on the idea of encapsulating data within structures, which people learned early on in programming
- however, it's not always the best idea, structures can be added to data without incapsulation
```c++
 struct Node {
    Node* next; 
};

struct MyData {
    int foo 
    Node Node; // embedded structure
};

Struct MultiIndexedData {
    int bar; 
    Node node1; // embedded structure 
    Node node2; // another structure 
};
```

in the example above, the actual node struct contains no data, instead, the data includes the structure 

```c++
size_t count_nodes(struct Node *node) {
    size_t cnt = 0;
    for (; node != nullptr; node = node->next) {
        cnt++; 
    }
    return cnt
}
```

unlike templated code, this code knows nothing about data types at all, its generic because the structure is independent of the data 

### Using Instrusive Data Strucutures
- with intrusive chaining hashtables, a lookup returns a list node instead of the data, the list node is part of teh data,
so th econtainer (data) can be found by offsetting the list node pointer, this is commonly done with the container_of macro 
```c++
Node *my_node = some_lookup_func();
MyData *my_data = container_of(my_node, MyData, node); 

#define container_of(ptr, T, member) ({ \
    const typeof( ((T&)0)->member) *__mptr) = (ptr);  // checks if ptr matches T::member with GCC extion typeof 
    (T *)( (char *)__mptr - offserof(T, member) ); }) // offesets ptr from T::member to T 
```

Advantages: 
- generic, but not aware of data types 
- viable in plain C 
- can have diff data types in same collection
- no internal mem mgmt, borrows space from data, no new allocations for heap-allocated dat a
- no assumptions about data ownership, share data between multiple collections without any indirection
Drawbacks: 
- needs typecasting, not as easy to use as templated collections 

## Chapter 9: Data Serialization 

- for now, our server protocol response is an error code plus a string, what if we ened to return more compicated data? 
- we could add the keys command, whicih returns a list of strings
- we have already encodede the list-of-strings data in teh request protocol, in this chapter, we will 
generalize the encoding to handle different types of data (serialization) 
- we will use TLV (type, length, value) serialization 

## Chapter 10: AVL Trees 

### Sorted Set for Sorting 

#### Range Query 
- a sorted set is acollection of (score, name) pairs indexed in 2 ways
1) find ths core by name, like a hashmap 
2) range query: getting a subset of the sorted pairs
- seek to the closest air froma  target(score, name) pair 
- iterate in ascending/descending order from the starting pair 

#### Rank Query
sorted set can also do queries related to the position in the list(rank)
- find a pairs rank 
- find a pair by its rank
- offset a pair, like offsetting a SQL query, these are o(log(n)) 

#### Sorted Set Use Cases 
suppose you have a high score board using sorted set, you can 
- find a users score 
- range query : show the list starting from a given score (pagination)
- rank query: find users rank/find user with given rank/paginate the list by offset instead of score

In addition to score boards, a sorted set can sort anything, suppose you have a list of posts 
ordered by time, you can: 
- find post time(score) by post ID(name) 
- range query: list posts starting from a given score 
- rank query: find pos of post in list, find post id at given pos, paginate list by offset instead of time 

### AVL Tree 
#### Invariant: Height difference only by one 

- an avl tree is a binary tree with an extra step, inserts and deletions are done as normal, then the tree is fixed by some rules 
- the height of a subtree is the max distance to leaves, for each node in an AVL tree, the height of its 2 subtrees can differ, but only by 1
- rules for fixing hiehgts is same for inserts/deletes 

#### Rotations keep the order/adjust heights 
- rotations change the shape of a subtree while keeping its order, visualization of a left rotation:
```
  2           4
 / \         / \
1   4  ==>  2   5
   / \     / \
  3   5   1   3
```
- inserting or removing a node changes the subtree height by 1, which can cause a height diff of 2 for the parent node, which is restored by rotations

Rule 1: A right totation restores balace if: 
- the left subtree(B) is deeper by 2 
- the left left subtree(A) is deeper than the left right subtree(C)
```
      D (h+3)               B (h+2)
     /      \              /      \
   B (h+2)   E (h)  ==>  A (h+1)   D (h+1)
  /      \                        /      \
A (h+1)   C (h)                  C (h)    E (h)
```

Rule 2: A right rorations makes the right subtree deper than the left subtree while keeping the rotationg height if: 
- the left subtree (B) is deeper by 1
```
      D (h+2)               B (h+2)
     /      \              /      \
   B (h+1)   E (h)  ==>  A (h-1)   D (h+1)
  /      \                        /      \
A (h-1)   C (h)                  C (h)    E (h)
```

- both rules are left-right mirrored, the left rotation rule 2 is paired with the right rotation rule 1

Steps: 
- check for imbalance (height diff of 2) , stop if balanced 
- if rule 1 is not application, apply opposite rotation rule 2
- apply rule 1 
- goto parent and repeat 

## Chapter 11: Sorted Set From AVL Tree 

### Sorted Set Commands 

we'll implement some simplified commands to cover all sorted set operations 
- insert a pair: ZADD key score name 
- find and remove by name: ZREM key name, ZSORE key name 
- range query: ZQUERY key score name offset limit (iterate thru sublist where pair >=(score,name), offset the sublist and limit the result size
- rank query: the offset in the ZQUERY command

In real redis, there are many range query commands 
- ZrangeByScore : range query by score without name 
- ZrangeByLex : assumes same score, range query by name 
- Zrange : query by rank, ZrangeByScore, or ZrangeByLex
- Zrev* : in descending order 
- Zrem* : range delete

Lots of work for the same functionality, so we invent the generic ZQUERY command, which can do everythign expcet delete ranges, 
reverse order, and the max arguement
- ZrangeByScore : ZQUERY with an empty name 
- ZrangeByLex : ZQUERY with the fixed score 
- ZrangeByRank : ZQUERY with (-inf, "") and offsets

## Chapter 12: Event Loop and Timers 

### Timeouts and Timers 

- one thing we are missing in our server is timeouts, every networked application needs to ahndle timeouts since
the other side of the network can just disappear 
- we need timeouts for not only ondoing i/o opertaions like read/write, but also a good idea to kcik out idle TCP connetions, 
- to implement timeouts, the event loop must be modified since the poll is the only thing that is blocking
```
int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000); 
```
- in our existing event loop, the poll syscall takes a timeout arg, which imposes an upper bound of time spent on the poll syscall 
- current an arbitrary value of 1000ms, if we set the timeout value accoding to the timer, poll should wake up at the time it expires, or before that, 
then we have a chance to fire the timer in due time 

- problem is we might have more than one timer, the timeout val of pol should be the timeout val of the nearest timer
- thus, some data strucutre is needed for finding the nearest timer, we can use a heap which is a good choice for finding the min/max val 
