#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>

#include <dc1394/dc1394_conversions.h>
#include <dc1394/dc1394_control.h>
#include <dc1394/dc1394_utils.h>

#include "stereohelper.h"
#include "socket.h"
#include "network.h"

using std::string;
using std::cout;
using std::endl;

void cleanup_and_exit( dc1394camera_t* camera,PGRStereoCamera_t* stereoCamera ) {
   if (stereoCamera != NULL) {
       //dc1394_capture_stop(stereoCamera->camera);
       //  Stop data transmission
       if ( dc1394_video_set_transmission( stereoCamera->camera, DC1394_OFF ) != DC1394_SUCCESS )
       {
           fprintf( stderr, "Couldn't stop the camera?\n" );
       }
       cout << "stopped transmission" << endl;
       //dc1394_free_camera(stereoCamera->camera);
   }

   dc1394_capture_stop( camera );
   dc1394_video_set_transmission( camera, DC1394_OFF );
   dc1394_free_camera( camera );
   exit(1);
}

dc1394camera_t* initialize_camera() {
    dc1394camera_t*  camera; 
    dc1394error_t err;    

    // Find cameras on the 1394 buses
    uint_t       nCameras;
    dc1394camera_t** cameras=NULL;
    err = dc1394_find_cameras( &cameras, &nCameras );

    if ( err != DC1394_SUCCESS ) 
    {
        fprintf( stderr, "Unable to look for cameras\n\n"
                "Please check \n"
                "  - if the kernel modules `ieee1394',`raw1394' and `ohci1394' are loaded \n"
                "  - if you have read/write access to /dev/raw1394\n\n");
        exit(2);
    }

    //  get the camera nodes and describe them as we find them
    if ( nCameras < 1 ) 
    {
        fprintf( stderr, "No cameras found!\n");
        exit(2);
    }

    unsigned int     nThisCam;
    printf( "There were %d camera(s) found attached to your PC\n", nCameras );
    for ( nThisCam = 0; nThisCam < nCameras; nThisCam++ )
    {
        printf( "Camera %d model = '%s'\n", nThisCam, cameras[nThisCam]->model );
        if ( isStereoCamera( cameras[nThisCam] ) )
        {
            printf( "Using this camera\n" );
            break;  
        }
    }

    if ( nThisCam == nCameras )
    {
        printf( "No stereo cameras were detected\n" );
        exit(2);
    }

    camera = cameras[nThisCam];

    // free the other cameras
    for ( unsigned int i = 0; i < nCameras; i++ )
    {
        if ( i != nThisCam )
            dc1394_free_camera( cameras[i] );
    }
    free(cameras);
    return camera;
}


void init_stereo_camera(PGRStereoCamera_t &stereoCamera, dc1394camera_t *camera) {
    dc1394error_t err;
    // query information about this stereo camera
    err = queryStereoCamera( camera, &stereoCamera );

    if ( err != DC1394_SUCCESS )
    {
        fprintf( stderr, "Cannot query all information from camera\n" );
        cleanup_and_exit(camera,&stereoCamera);
    }

    // set the capture mode
    printf( "Setting stereo video capture mode\n" );
    err = setStereoVideoCapture( &stereoCamera );
    if ( err != DC1394_SUCCESS )
    {
        fprintf( stderr, "Could not set up video capture mode\n" );
        cleanup_and_exit(camera,&stereoCamera);
    }

    // have the camera start sending us data
    printf( "Start transmission\n" );
    err = startTransmission( &stereoCamera );
    if ( err != DC1394_SUCCESS )
    {
        fprintf( stderr, "Unable to start camera iso transmission\n" );
        cleanup_and_exit(camera,&stereoCamera);
    }
    return;
}


// return center rgb arr, height x width x 3 x n_bytes
unsigned char* grab_one_frame(dc1394camera_t*  camera, PGRStereoCamera_t *stereoCamera,
        unsigned char* pucDeInterlacedBuffer, unsigned char* pucRGBBuffer,
        unsigned char* pucGreenBuffer) {
    if ( dc1394_capture_dma( &stereoCamera->camera, 1, DC1394_VIDEO1394_WAIT )!= DC1394_SUCCESS)
    {
        fprintf( stderr, "Unable to capture a frame\n" );
        cleanup_and_exit(camera,stereoCamera);
    }
    // allocate a buffer to hold the de-interleaved images
    unsigned char* pucRightRGB    = NULL;
    unsigned char* pucLeftRGB     = NULL;
    unsigned char* pucCenterRGB   = NULL;
    TriclopsInput input;

    // get the images from the capture buffer and do all required processing
    // note: produces a TriclopsInput that can be used for stereo processing
    extractImagesColor( stereoCamera,
            DC1394_BAYER_METHOD_NEAREST,
            pucDeInterlacedBuffer,
            pucRGBBuffer,
            pucGreenBuffer,
            &pucRightRGB,
            &pucLeftRGB,
            &pucCenterRGB,
            &input );
    return pucLeftRGB;
}


void compress(int height, int width, unsigned char *orig, unsigned char *down) {
    int dh=height/2;
    int dw=width/2;
    //downsample the image to 0.5 orig h and w
    for (int h=0;h<dh;h++) {
        for (int w=0;w<dw;w++) {
            down[3*h*dw+3*w]=orig[3*2*h*width+3*2*w];
            down[3*h*dw+3*w+1]=orig[3*2*h*width+3*2*w+1];
            down[3*h*dw+3*w+2]=orig[3*2*h*width+3*2*w+2];
        }
    }
    return;
}


// Entry point: Set up the game, create jobs, then run the scheduler
int main(int argc, char** argv) {

    dc1394camera_t* camera = initialize_camera(); 
    PGRStereoCamera_t stereoCamera;
    init_stereo_camera(stereoCamera,camera);

    // Declare sockets and port
    int server_socket_fd;
    unsigned short server_port;
    int socket_fd;

    // Initialize the home game
    open_camera_connection(&server_port, &server_socket_fd);
    cout << "port number: " << server_port << endl;

    // Accept connection
    accept_connection(server_socket_fd, &socket_fd);
    cout << "accepted connection" << endl; 

    // send img height
    int ret;
    int height = stereoCamera.nRows;
    int width = stereoCamera.nCols;
    int nbytes = stereoCamera.nBytesPerPixel;

    cout << height << " " << width << " " << nbytes << endl; 

    int dh=height/2;
    int dw=width/2;

    ret = send_int(socket_fd, dh);
    if (ret != 0) {
        fprintf( stderr, "Unable to send height\n" );
        cleanup_and_exit(camera,&stereoCamera);
    }
    cout << "sent height" << endl;
    ret = send_int(socket_fd, dw);
    if (ret != 0) {
        fprintf( stderr, "Unable to send width\n" );
        cleanup_and_exit(camera,&stereoCamera);
    }
    cout << "sent width" << endl;

    unsigned int   nBufferSize = height*width*nbytes;
    unsigned char* pucDeInterlacedBuffer = new unsigned char[ nBufferSize ];
    unsigned char* pucRGBBuffer   = new unsigned char[ 3 * nBufferSize ];
    unsigned char* pucGreenBuffer     = new unsigned char[ nBufferSize ];

    unsigned char *down = (unsigned char *)malloc(dh*dw*3*sizeof(unsigned char *));
    while (true) {
        //usleep(1000000); //micro secs
        unsigned char *img_arr = grab_one_frame(camera,&stereoCamera,
                pucDeInterlacedBuffer,pucRGBBuffer,pucGreenBuffer);
        compress(height,width,img_arr,down);
        
        /*
        unsigned char test[768*1024*3]={0};
        for (int i=50;i<250;i++) {
            for (int j=50;j<250;j++) {
                test[i*width*3+j*3]=255;
                test[i*width*3+j*3+1]=255;
                test[i*width*3+j*3+2]=255;
            }
        }
        */
        //ret = send_image(socket_fd,height,width,img_arr);
        ret = send_image(socket_fd,dh,dw,down);
        if (ret != 0) {
            fprintf( stderr, "Unable to send image\n" );
            dc1394_capture_dma_done_with_buffer(stereoCamera.camera);
            //cleanup_and_exit(camera,&stereoCamera);
            break;
        }
        dc1394_capture_dma_done_with_buffer(stereoCamera.camera);
        /*
        int cf = get_int(socket_fd);
        if (cf != 0) {
            fprintf( stderr, "Did not get confirmation\n" );
            //dc1394_capture_dma_done_with_buffer(stereoCamera.camera);
            //cleanup_and_exit(camera,&stereoCamera);
            break;
        }
        */
        //break;
    }

    // close camera
    cleanup_and_exit(camera,&stereoCamera);

    return 0;
}
