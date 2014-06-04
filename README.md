Online multiplayer proof-of-concept

Framework for developing Super Smash Bros - like multiplayer games that are playable online.

For now this works in a simulated network environment, using a custom socket class (netsim.h
and netsim.cpp).

To run, compile the server - while setting the parameter of the server object's constructor
in main.cpp as desired. Then compile a separate client for each player, setting them up with
different simulated IP addresses and names (client's main.cpp), and keyboard controls (in
client.cpp).

(Yes, I do realize it's cumbersome.)

Video in action:
Related blog post:
