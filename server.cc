#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

#include <dc1394/dc1394_conversions.h>
#include <dc1394/dc1394_control.h>
#include <dc1394/dc1394_utils.h>

#include "stereohelper.h"
#include "socket.h"
#include "network.h"

using std::string;
using std::cout;
using std::endl;

void cleanup_and_exit( dc1394camera_t* camera ) {
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
		return 1;
	}

	//  get the camera nodes and describe them as we find them
	if ( nCameras < 1 ) 
	{
		fprintf( stderr, "No cameras found!\n");
		return 1;
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
		return 0;
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
		cleanup_and_exit( camera );
	}

	// set the capture mode
	printf( "Setting stereo video capture mode\n" );
	err = setStereoVideoCapture( &stereoCamera );
	if ( err != DC1394_SUCCESS )
	{
		fprintf( stderr, "Could not set up video capture mode\n" );
		cleanup_and_exit( stereoCamera.camera );
	}

	// have the camera start sending us data
	printf( "Start transmission\n" );
	err = startTransmission( &stereoCamera );
	if ( err != DC1394_SUCCESS )
	{
		fprintf( stderr, "Unable to start camera iso transmission\n" );
		cleanup_and_exit(stereoCamera.camera );
	}
    return;
}


// return center rgb arr, height x width x 3 x n_bytes
unsigned char* grab_one_frame(PGRStereoCamera_t *stereoCamera) {
    if ( dc1394_capture_dma( &stereoCamera->camera, 1, DC1394_VIDEO1394_WAIT )!= DC1394_SUCCESS)
    {
        fprintf( stderr, "Unable to capture a frame\n" );
        cleanup_and_exit( stereoCamera->camera );
    }
    unsigned int   nBufferSize = stereoCamera->nRows *
        stereoCamera->nCols *
        stereoCamera->nBytesPerPixel;
    // allocate a buffer to hold the de-interleaved images
    unsigned char* pucDeInterlacedBuffer = new unsigned char[ nBufferSize ];
    unsigned char* pucRGBBuffer   = new unsigned char[ 3 * nBufferSize ];
    unsigned char* pucGreenBuffer     = new unsigned char[ nBufferSize ];
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
    return pucCenterRGB;
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

    // send img height
    int ret;
    int height = stereoCamera.nRows;
    int width = stereoCamera.nCols;
    int nbytes = stereoCamera.nBytesPerPixel;
    ret = send_int(socket_fd, height);
    if (ret != 0) {
        fprintf( stderr, "Unable to send height\n" );
        cleanup_and_exit( stereoCamera.camera );
    }
    ret = send_int(socket_fd, width);
    if (ret != 0) {
        fprintf( stderr, "Unable to send width\n" );
        cleanup_and_exit( stereoCamera.camera );
    }
    ret = send_int(socket_fd, nbytes);
    if (ret != 0) {
        fprintf( stderr, "Unable to send nbytes\n" );
        cleanup_and_exit( stereoCamera.camera );
    }

    // Wait for the game to end
    while (true) {
        unsigned char *img_arr = grab_one_frame(&stereoCamera);
        ret = send_image(socket_fd,height,width,nbytes,img_arr);
        if (ret != 0) {
            fprintf( stderr, "Unable to send image\n" );
            cleanup_and_exit( stereoCamera.camera );
        }
        break;
    }

    //  Stop data transmission
    if ( dc1394_video_set_transmission( stereoCamera.camera, DC1394_OFF ) != DC1394_SUCCESS )
    {
        fprintf( stderr, "Couldn't stop the camera?\n" );
    }
    // close camera
    cleanup_and_exit( camera );

    return 0;
}
