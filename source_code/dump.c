#include <stdio.h>
#include <vl/generic.h>
#include <vl/sift.h>
#include <OpenCV/cv.h>
#include <OpenCV/highgui.h>
#define dprintf //printf

VlSiftKeypoint* mySift(IplImage* src);

int main (int argc, const char * argv[]) {
	char *imagefilename=(char*)malloc(sizeof(char)*16);
	char *dscfilename=(char*)malloc(sizeof(char)*16);
	if (argc<3) {
		printf("Usage: ./dump-descr image-file-name descriptor-file-name");
		strcpy(imagefilename, "savekkkk.jpg");
		strcpy(dscfilename, "saveD.jpg.dsc");
	}
	else {
		strcpy(imagefilename,argv[1]);
		strcpy(dscfilename,argv[2]);
	}
	
	FILE* dscfile;
	int w=1280,h=720;
	int i=0;
	int nkeypoints=0;
	vl_bool render=1;
	vl_bool first=1;
	VlSiftFilt * myFilter=0;
	VlSiftKeypoint const* keys;
	char img2_file[] = "/Users/quake0day/ana2/MVI_0124.MOV";
	
	//printf("sizeof(VlSiftKeypoint)=%d, filt=%d, pix=%d\n", sizeof(VlSiftKeypoint), sizeof(VlSiftFilt),sizeof(vl_sift_pix));
	
	dscfile=fopen(dscfilename, "wb");
	vl_sift_pix* fim;
	int err=0;
	int octave, nlevels, o_min;
	
	//vl_sift_pix descr[128];
	
	
	//CvCapture * camera = cvCreateCameraCapture (CV_CAP_ANY);
	CvCapture * camera = cvCreateFileCapture(img2_file);

	cvNamedWindow("Hello", 1);
	
	IplImage *myCVImage=cvQueryFrame(camera);//cvLoadImage(imagefilename, 0);
	
	IplImage *afterCVImage=cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 1);
	IplImage *resizingImg=cvCreateImage(cvSize(w, h), myCVImage->depth, myCVImage->nChannels);
	octave=3;
	nlevels=10;
	o_min=1;
	myFilter=vl_sift_new(w, h, octave, nlevels, o_min);
	vl_sift_set_peak_thresh(myFilter, 0.5);
	fim=malloc(sizeof(vl_sift_pix)*w*h);
	int press=0;
	
	while (myCVImage) {
		
		dprintf("%d*%d\n",myCVImage->width,myCVImage->height);
		//w=myCVImage->width;
		//h=myCVImage->height;
		
		cvResize(myCVImage, resizingImg, CV_INTER_AREA);
		dprintf("resized scale:%d*%d\n",myCVImage->width,myCVImage->height);
		cvConvertImage(resizingImg, afterCVImage, 0);
		
		
		for (i=0; i<h; i++) {
			for (int j=0; j<w; j++) {
				fim[i*w+j]=CV_IMAGE_ELEM(afterCVImage,uchar,i,j);
				//printf("%f ", fim[i*w+j]);
			}
		}
		
		
		//vl_sift_set_peak_thresh(myFilter, 0.5);
		//vl_sift_set_edge_thresh(myFilter, 10.0);
		first=1;
		while (1) {
			if (first) {
				first=0;
				err=vl_sift_process_first_octave(myFilter, fim);
			}
			else {
				err=vl_sift_process_next_octave(myFilter);
			}
			if (err) {
				err=VL_ERR_OK;
				break;
			}
			
			vl_sift_detect(myFilter);
			nkeypoints=vl_sift_get_nkeypoints(myFilter);
			dprintf("insider numkey:%d\n",nkeypoints);
			keys=vl_sift_get_keypoints(myFilter);
			dprintf("final numkey:%d\n",nkeypoints);
			
			
			if (render) {
				for (i=0; i<nkeypoints; i++) {
					cvCircle(resizingImg, cvPoint(keys->x, keys->y), keys->sigma, cvScalar(100, 255, 50, 0), 1, CV_AA, 0);
					//printf("x:%f,y:%f,s:%f,sigma:%f,\n",keys->x,keys->y,keys->s,keys->sigma);
					if (press=='d') {
						
						double angles [4] ;
						int nangles ;
						
						/* obtain keypoint orientations ........................... */
						nangles=vl_sift_calc_keypoint_orientations(myFilter, angles, keys);
						
						/* for each orientation ................................... */
						for (int q = 0 ; q < (unsigned) nangles ; ++q) {
							vl_sift_pix descr [128] ;
							
							
							//printf("\n");
							/* compute descriptor (if necessary) */
							vl_sift_calc_keypoint_descriptor(myFilter, descr, keys, angles[q]);
							for (int j=0; j<128; j++) {
								descr[j]*=512.0;
								descr[j]=(descr[j]<255.0)?descr[j]:255.0;
								printf("%f ", descr[j]);
							}
							fwrite(descr, sizeof(vl_sift_pix), 128, dscfile);
						}
					}
					keys++;
				}
			}
			
		}
		
		cvShowImage("Hello", resizingImg);
		
		myCVImage = cvQueryFrame(camera);
		
		press=cvWaitKey(1);
		if( press=='q' )
			return 0;
		else if( press=='r' )
			render=1-render;
	}
	free(fim);
	cvReleaseImage(&afterCVImage);
	cvReleaseImage(&resizingImg);
	cvReleaseImage(&myCVImage);
	
	return 0;
}
