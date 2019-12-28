# chord
Chord implementation in C (Wrote for Linux). Supports everything but finger tables as of right now. Resilient to -r node failures. Handles many nodes joining with updates to successor lists and makes routing choices based on successor lists.

**Protocol:**

https://people.eecs.berkeley.edu/~istoica/papers/2003/chord-ton.pdf

Applications may then be built on top of this service that Chord provides. For example, a
distributed hash table (DHT) application may distribute the key-value storage of a hash table
across many peers and support the typical operations (i.e., insert, get, and remove). The DHT
may be implemented on top of Chord by storing the key-value pairs at the node that Chord mapped
to the given key.

![GitHub Logo](/chord.png)

**To run:**

  1) Install google protocol buffers:
  
    sudo apt-get install libprotobuf-c-dev protobuf-c-compiler
    
  2) Execute:
  
   An example usage to start a new Chord ring is:
    
      chord -a 128.8.126.63 -p 4170 --ts 30000 --tff 10000 --tcp 30000 -r 4
    
   An example usage to join an existing Chord ring is:
    
      chord -a 128.8.126.63 -p 4171 --ja 128.8.126.63 --jp 4170 --ts 30000 --tff 10000 --tcp 30000 -r 4
    
   1. -a <String> = The IP address that the Chord client will bind to, as well as advertise to
    other nodes. Represented as an ASCII string (e.g., 128.8.126.63). Must be specified.
    
   2. -p <Number> = The port that the Chord client will bind to and listen on. Represented as
    a base-10 integer. Must be specified.
    
   3. --ja <String> = The IP address of the machine running a Chord node. The Chord client
    will join this node’s ring. Represented as an ASCII string (e.g., 128.8.126.63). Must be
    specified if --jp is specified.
    
   4. --jp <Number> = The port that an existing Chord node is bound to and listening on. The
    Chord client will join this node’s ring. Represented as a base-10 integer. Must be specified if
    --ja is specified.
    
   5. --ts <Number> = The time in milliseconds between invocations of ‘stabilize’. Represented
    as a base-10 integer. Must be specified, with a value in the range of [1,60000].
    
   6. --tff <Number> = The time in milliseconds between invocations of ‘fix fingers’. Represented as a base-10 integer. Must        be specified, with a value in the range of [1,60000].
    
   7. --tcp <Number> = The time in milliseconds between invocations of ‘check predecessor’.
    Represented as a base-10 integer. Must be specified, with a value in the range of [1,60000].
    
   8. -r <Number> = The number of successors maintained by the Chord client. Represented as
    a base-10 integer. Must be specified, with a value in the range of [1,32].
    
   9. -i <String> = The identifier (ID) assigned to the Chord client which will override the ID
    computed by the SHA1 sum of the client’s IP address and port number. Represented as a
    string of 40 characters matching [0-9a-fA-F]. Optional parameter.
  
  **Commands:**
  
  There are two command that the Chord client must support: ‘Lookup’ and ‘PrintState’.
  
  **‘Lookup’** takes as input an ASCII string (e.g., “Hello”). The Chord client takes this string,
  hashes it to a key in the identifier space, and performs a search for the node that is the successor
  to the key (i.e., the owner of the key). The Chord client then outputs that node’s identifier, IP
  address, and port.
  
  **‘PrintState’** requires no input. The Chord client outputs its local state information at the
  current time, which consists of:
  1. The Chord client’s own node information
  2. The node information for all nodes in the successor list
  3. The node information for all nodes in the finger table
  
  ![GitHub Logo](/commands.png)

  
    
