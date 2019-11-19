#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

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
  int nbytes = get_int(socket_fd);

  while (true) {
      unsigned char *arr=get_one_picture(socket_fd,height,width,nbytes);
      Mat mat = arr2Mat(height,width,arr);
      save_img("test.jpg",mat);
      break;
  }

  return 0;
}
