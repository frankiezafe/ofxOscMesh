ofxOscMesh
==========

Openframeworks addons for mesh network.

Deploy in [of]/addons/ofxOscMesh.

See [iterations 2016](https://gitlab.com/frankiezafe/iterations2016/) for a complete project description. This addon has been done to broadcast tags seen with [ofxARtoolKitPlus](https://github.com/fishkingsin/ofxARtoolkitPlus).

Communication protocol
======================

The communication in this project is based on a mesh network model. Each point (~application) is independant from the others and is able to ask to the others what it wants from them.

This communication uses 2 [UDP](https://en.wikipedia.org/wiki/User_Datagram_Protocol) ports on each point:

+ heartbeat (default port: 20000, default beat: each 5 seconds)
+ data (default port: 23000)

Messages are packed using [OSC](http://opensoundcontrol.org/).

see Iteration/src/Config.cpp for default values.


heartbeat
---------

Each point emits a "hello world" message each 5 seconds. By doing so, all the other points in the network keep the communication open with it.

This message has to be structured has followed to be considered as valid:

+ address: */a*
+ arg[0]: *string* - *mandatory*, IP, for instance "192.168.1.24"
+ arg[1]: *int* - *mandatory*, data port, 23000 by default
+ arg[2]: *int* - optional, datakind, see below for explanations
+ arg[3]: *string* - optional, name of the point, for instance "artoolkit_near_the_door"

After a certain amount of time (20 seconds by default) without receiving an heartbeat from a point, the other points will consider the point dead and will stop sending it data.


datakind
--------

Datakind is an integer of 8 bits describing the data to send or the data received. Each of its bits is used has a boolean.

A datakind can be read as follows:
+ if datakind[ 0 ] == true > xy coordinates of artoolkit's tag
+ if datakind[ 1 ] == true > xyz coordinates of artoolkit's tag
+ if datakind[ 2 ] == true > rotation Z of artoolkit's tag
+ if datakind[ 3 ] == true > matrix of artoolkit's tag
+ if datakind[ 4 ] == true > free slot
+ if datakind[ 5 ] == true > free slot
+ if datakind[ 6 ] == true > free slot
+ if datakind[ 7 ] == true > free slot

In interger, it corresponds to:
+ [0] ~ 1
+ [1] ~ 2
+ [2] ~ 4
+ [3] ~ 8
+ [4] ~ 16
+ [5] ~ 32
+ [6] ~ 64
+ [7] ~ 128

Via heartbeats, each point inform the network about wich kind of data it can provide.


data
----

Data messages are of two kinds:

+ address: */r*, containing *requests* to emit data
+ address: */d*, containing data from other points


data messages /r
----------------

If a point (the consumer) wants to get data from another points (the producer), it asks the producer to open a datastream. This request must go through the *data* port of the producer and be formed as follows:

+ address: */r*
+ arg[0]: *string* - *mandatory*, IP of the consumer
+ arg[1]: *int* - *mandatory*, data port of the consumer
+ arg[2]: *int* - *mandatory*, output datakind
+ arg[2]: *int* - *mandatory*, request datakind

Once received, the producer packs messages following the datakind requested by the consumer.

*Example*

The consumer wants to receive the *xy* coordinates and the *z rotation* of tags, the request's datakind will be *5* ([0] && [2], translated into int > 1+4 ).

Note that consumer must emits heartbeats for its data requests to be accepted.


data messages /d
----------------

By default, the producer are not sending information to other points. 

Once the communication enabled, the producer is creating custom messages for each consumer, using this format:

+ address: */d*
+ arg[0]: *int* - datakind, describing the meaning of the following arguments
+ arg[1]: *string* - name of the producer
+ arg[2]: *int* - tag ID
+ arg[3]: *int* - event type, [0,1,2] - [first apparition, presence, disparition]
+ arg[3+]: data
  + if xy enabled, adding 2 floats
  + if xyz enabled, adding 3 floats
  + if rotation z enabled, adding 1 float
  + if matrix enabled, adding 16 floats


