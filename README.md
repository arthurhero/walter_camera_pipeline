# Walter's camera pipeline

This is a data stream pipeline for sending images captured by the stereo camera on Walter to another computer via network in real time. 

## Usage

Clone this folder to Walter's PC. `cd` into this folder, run `make server`.

Clone this folder to the target workstation (possibly a computer with GPU). `cd` into this folder, run `make client`.

Run `./server` on Walter. You will the port number displayed on the screen.

Then run `./client walter <port number>` on the workstation.

You will see a window pop out, displaying the video stream in real-time.

You can also modify the code in `client.cc` to profile the data speed. Instructions are given inside the file.

## Files

*socket.h*: a wrapper for the system's socket library authored by Charlie.

*network.h*: all the networking functions including sending and receiving integers, sending and receiving images, opening and joining socket.

*server.cc*: the main code for server (Walter), including functions dealing with the BumbleBee2 camera.

*client.cc*: the main code for client (workstation), including functions for displaying images and saving images.

## Unfinished work

Need to complete the C++ to Python bridge since the machine learning models will be in Python.
