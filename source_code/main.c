#include <stdio.h>
#include <vl/generic.h>
#include <vl/sift.h>
#include <OpenCV/cv.h>
#include <OpenCV/highgui.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <vl/kdtree.h>
#include <vl/mathop.h>
#define dprintf //printf
#define printf printf
#define DESCSIZE 128



int main (int argc, const char * argv[]) {
	int w=1280,h=720;
	int i=0;
	int nkeypoints=0;
	int press=0;
	char img2_file[] = "/Users/quake0day/ana2/MVI_0124_QT 768Kbps_012.mov";
	vl_bool render=1;
	vl_bool first=1;
	VlSiftFilt * myFilter=0;
	VlSiftKeypoint const* keys;
	//CvCapture * camera = cvCreateCameraCapture (CV_CAP_ANY);
	CvCapture * camera = cvCreateFileCapture(img2_file);
	vl_sift_pix *descriptorsA, *descriptorsB;
	int ndescA=0, ndescB=0;
	
	//DescriptorA file
	int dscfd;
	struct stat filestat;
	dscfd = open("/Users/quake0day/ana2/saveC.jpg.dsc", O_RDONLY, 0644);
	fstat(dscfd, &filestat);
	int filesize=filestat.st_size;
	descriptorsA=(vl_sift_pix*)mmap(0, filesize, PROT_READ, MAP_SHARED, dscfd, 0);
	ndescA=(filesize/sizeof(vl_sift_pix))/DESCSIZE;
	printf("number of descriptors: %d\n", ndescA);
	
	//Build kdtreeA
	VlKDForest *myforest=vl_kdforest_new(VL_TYPE_FLOAT, DESCSIZE, 1);
	vl_kdforest_build(myforest, ndescA, descriptorsA);
	
	//DescriptorsB file
	dscfd=open("/Users/quake0day/ana2/saveD.jpg.dsc", O_RDONLY, 0644);
	fstat(dscfd, &filestat);
	filesize=filestat.st_size;
	descriptorsB=(vl_sift_pix*)mmap(0, filesize, PROT_READ, MAP_SHARED, dscfd, 0);
	ndescB=(filesize/sizeof(vl_sift_pix))/DESCSIZE;
	printf("number of descriptors: %d\n", ndescB);
	
	//Build kdtreeB
	VlKDForest *myforestB=vl_kdforest_new(VL_TYPE_FLOAT, DESCSIZE, 1);
	vl_kdforest_build(myforestB, ndescB, descriptorsB);
	
	//Neighbors
	VlKDForestNeighbor *neighbors=(VlKDForestNeighbor*)malloc(sizeof(VlKDForestNeighbor));
	VlKDForestNeighbor *neighborsB=(VlKDForestNeighbor*)malloc(sizeof(VlKDForestNeighbor));
	
	//Image variables
	vl_sift_pix* fim;
	int err=0;
	int octave, nlevels, o_min;
	cvNamedWindow("Hello", 1);
	
	//For text
	CvFont font;
	double hScale=2;
	double vScale=2;
	int    lineWidth=2;
		cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX|CV_FONT_ITALIC, hScale,vScale,0,lineWidth, 1);
	
	IplImage *myCVImage=cvQueryFrame(camera);//cvLoadImage("2.jpg", -1);
	
	IplImage *afterCVImage=cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 1);
	IplImage *resizingImg=cvCreateImage(cvSize(w, h), myCVImage->depth, myCVImage->nChannels);
	octave=2;
	nlevels=5;
	o_min=1;
	myFilter=vl_sift_new(w, h, octave, nlevels, o_min);
	vl_sift_set_peak_thresh(myFilter, 0.5);
	fim=malloc(sizeof(vl_sift_pix)*w*h);
	float thre;
	
	
	while (myCVImage) {
		dprintf("%d*%d\n",myCVImage->width,myCVImage->height);
		
		cvResize(myCVImage, resizingImg, CV_INTER_AREA);
		dprintf("resized scale:%d*%d\n",myCVImage->width,myCVImage->height);
		if (press=='s') {
			cvSaveImage("save.jpg", resizingImg);
		}
		cvConvertImage(resizingImg, afterCVImage, 0);
		
		
		for (i=0; i<h; i++) {
			for (int j=0; j<w; j++) {
				fim[i*w+j]=CV_IMAGE_ELEM(afterCVImage,uchar,i,j);
			}
		}
		
		//vl_sift_set_peak_thresh(myFilter, 0.5);
		//vl_sift_set_edge_thresh(myFilter, 10.0);
		first=1;
		while (1) {
			printf("~~~~~~~~~~start of octave~~~~~~~~~~~~\n");
			
			
			if (first) {
				first=0;
				thre=0.25;
				err=vl_sift_process_first_octave(myFilter, fim);
			}
			else {
				thre=0.05;
				err=vl_sift_process_next_octave(myFilter);
			}
			if (err) {
				err=VL_ERR_OK;
				break;
			}
			
			printf("Octave: %d\n", vl_sift_get_octave_index(myFilter));
			vl_sift_detect(myFilter);
			nkeypoints=vl_sift_get_nkeypoints(myFilter);
			dprintf("insider numkey:%d\n",nkeypoints);
			keys=vl_sift_get_keypoints(myFilter);
			dprintf("final numkey:%d\n",nkeypoints);
			
			int countA=0, countB=0;
			int matchcountA=0, matchcountB=0;
			float avgA=0, avgB=0;
			if (render) {
				for (i=0; i<nkeypoints; i++) {
					//cvCircle(resizingImg, cvPoint(keys->x, keys->y), keys->sigma, cvScalar(100, 255, 50, 0), 1, CV_AA, 0);
					dprintf("x:%f,y:%f,s:%f,sigma:%f,\n",keys->x,keys->y,keys->s,keys->sigma);
					
					double angles [4] ;
					int nangles ;
					
					
					/* obtain keypoint orientations ........................... */
					nangles=vl_sift_calc_keypoint_orientations(myFilter, angles, keys);
					
					/* for each orientation ................................... */
					for (int q = 0 ; q < (unsigned) 1 ; ++q)
					{
						vl_sift_pix descr [128] ;
						
						
						/* compute descriptor (if necessary) */
						vl_sift_calc_keypoint_descriptor(myFilter, descr, keys, angles[q]);
						
						for (int j=0; j<128; j++)
						{
							descr[j]*=512.0;
							descr[j]=(descr[j]<255.0)?descr[j]:255.0;
						}
						
						vl_kdforest_query(myforest, neighbors, 1, descr);
						vl_kdforest_query(myforestB, neighborsB, 1, descr);
						if (neighbors->distance<50000.0)
						{
							matchcountA++;
							cvCircle(resizingImg, cvPoint(keys->x, keys->y), keys->sigma, cvScalar(100, 0, 0, 255), 1, CV_AA, 0);
							
						}
						
						if (neighborsB->distance<50000.0)
						{
							matchcountB++;
							cvCircle(resizingImg, cvPoint(keys->x, keys->y), keys->sigma, cvScalar(0, 50, 255, 100), 1, CV_AA, 0);
							
						}
						
						countA++;
						avgA+=neighbors->distance;
						countB++;
						avgB+=neighborsB->distance;
						
					}
					keys++;
				}
			}
			avgA=avgA/countA;
			float percentage=((float)matchcountA*2)/ndescA;
			printf("Percentage:%f\n", percentage);
			printf("avg:%f\n",avgA);
			printf("thre==%f\n", thre);
			if (percentage>=thre) {
				printf("A shows!!!\n");
				cvPutText (resizingImg, "A shows!!",cvPoint(50, 100), &font, cvScalar(0,255,255,0));
				
			}
			
			avgB=avgB/countB;
			percentage=((float)matchcountB*2.5)/ndescB;
			printf("Percentage:%f\n", percentage);
			printf("avg:%f\n",avgB);
			printf("thre==%f\n", thre);
			if (percentage>=thre) {
				printf("B shows!!!\n");
				cvPutText (resizingImg, "B shows!!",cvPoint(400, 100), &font, cvScalar(0,255,255,0));
				
			}
			printf("~~~~~~~~~~~end of octave~~~~~~~~~~~~\n");
		}
		
		cvShowImage("Hello", resizingImg);
		
		myCVImage = cvQueryFrame(camera);
		
		press=cvWaitKey(1);
		if( press=='q' )
			break;
		else if( press=='r' )
			render=1-render;
	}
	free(fim);
	free(neighbors);
	free(neighborsB);
	cvReleaseImage(&afterCVImage);
	cvReleaseImage(&resizingImg);
	cvReleaseImage(&myCVImage);
	
    return 0;
}
