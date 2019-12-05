#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <iostream>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/mat.hpp>   // cv::Mat

#include "socket.h"
#include "network.h"

using std::string;
using std::cout;
using std::endl;
using cv::Mat;

Mat arr2Mat(int height, int width, unsigned char *arr) {
    Mat mat=Mat(height, width, CV_8UC3, arr);
    Mat channel[3];
    Mat channel_[3];
    cv::split(mat,channel);
    channel_[0]=channel[2];
    channel_[1]=channel[1];
    channel_[2]=channel[0];
    cv::merge(channel_,3,mat);
    return mat;
}

void save_img(const string filename, Mat mat) {
    cv::imwrite(filename, mat);
    return;
}

int main(int argc, char** argv) {

  // Check the argument number is correct
  if(argc != 3) {
      fprintf(stderr, "Usage: %s [<server name> <port number>]\n", argv[0]);
      exit(1);
  }

  // Declare socket fd 
  int socket_fd;

  // Unpack arguments
  char* server_hostname = argv[1];
  unsigned short server_port= atoi(argv[2]);

  // Join the host's game
  join_camera_connection(server_hostname, server_port, &socket_fd);
  cout << "joined camera connection!" << endl;

  int height = get_int(socket_fd);
  int width = get_int(socket_fd);

  cv::namedWindow("display",cv::WINDOW_AUTOSIZE);

  cout << height << " " << width <<endl;

  int counter=0;
  int start = time(NULL);
  unsigned char *arr = (unsigned char *)calloc(height * width * 3, sizeof(unsigned char));
  while (true) {
  //while (counter<100) {
      get_one_picture(socket_fd,arr,height,width);
      //cout << "got" << endl;
      /*
      */
      Mat mat = arr2Mat(height,width,arr);
      cv::imshow("display",mat);
      cv::waitKey(1);
      //save_img("test.jpg",mat);
      //break;
      counter++;
      //send_int(socket_fd,0);
  }
  int end= time(NULL);
  cout << "total sec: " << end-start << " for " << counter << " pics " << endl;

  return 0;
}
