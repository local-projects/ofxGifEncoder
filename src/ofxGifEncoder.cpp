//
//  ofxGifEncoder.cpp
//  ofxGifEncoder
//
//  Created by Jesus Gollonet on 3/20/11.

#include "ofxGifEncoder.h"
#include "FreeImage.h"
// for some reason we're not seeing this from freeimage
#define DWORD uint32_t

ofxGifEncoder::ofxGifEncoder() {
}

void ofxGifEncoder::setup(int _w, int _h, int _nColors, float _frameDuration){
    w       = _w;
    h       = _h;
    if (nColors < 2 || nColors > 256) {
        ofLog(OF_LOG_WARNING, "ofxGifEncoder: nColors must be between 2 and 256. set to 256");
        nColors = 256;
    }
    
    nColors         = _nColors;
    frameDuration   = _frameDuration;
    bSaving         = false;
    start(); // does this have to be inited here?
}

ofxGifEncoder::~ofxGifEncoder() {}


//--------------------------------------------------------------
ofxGifFrame * ofxGifEncoder::createGifFrame(unsigned char * px, int _w, int _h, float duration, int bitsPerPixel) {
    ofxGifFrame * gf    = new ofxGifFrame();
    gf->pixels          = px;
    gf->width           = _w; 
    gf->height          = _h; 
    gf->duration        = duration;
    gf->bitsPerPixel    = bitsPerPixel;
    return gf;
}

void ofxGifEncoder::addFrame(ofImage & img, float _duration, int _bitsPerPixel) {
    if(img.width != w || img.height != h) {
        ofLog(OF_LOG_WARNING, "ofxGifEncoder::addFrame image dimensions don't match, skipping frame");
        return;
    }
    addFrame(img.getPixels(), w, h, _duration, _bitsPerPixel);
}

void ofxGifEncoder::addFrame(unsigned char *px, int _w, int _h, float _duration, int _bitsPerPixel) {
    if(_w != w || _h != h) {
        ofLog(OF_LOG_WARNING, "ofxGifEncoder::addFrame image dimensions don't match, skipping frame");
        return;
    }
    
    unsigned char * temp = new unsigned char[w * h * 3];
    memcpy(temp, px, w * h * 3);
    ofxGifFrame * gifFrame   = ofxGifEncoder::createGifFrame(temp, w, h, _duration) ;
    frames.push_back(gifFrame);
}



//--------------------------------------------------------------
void ofxGifEncoder::save (string _fileName) {
    fileName = _fileName;
    
    
    bSaving = true;
    
}


//--------------------------------------------------------------
void ofxGifEncoder::threadedFunction() {
	while( isThreadRunning() != 0 ) {
		if( lock() ){
			update();
			unlock();
			ofSleepMillis(10);
		}
	}
}

//--------------------------------------------------------------
void ofxGifEncoder::update() {
    if (bSaving) {
        doSave();
        bSaving = false;
    }
}

void ofxGifEncoder::doSave() {
	// create a multipage bitmap
	FIMULTIBITMAP *multi = FreeImage_OpenMultiBitmap(FIF_GIF, ofToDataPath(fileName).c_str(), TRUE, FALSE); 
	FIBITMAP * bmp = NULL;
	
	for(int i = 0; i < frames.size(); i++ ) { 
		// we need to swap the channels if we're on little endian (see ofImage::swapRgb);
#ifdef TARGET_LITTLE_ENDIAN
        swapRgb(frames[i]);
#endif
		
		// get the pixel data
		bmp	= FreeImage_ConvertFromRawBits(
            frames[i]->pixels, 
            frames[i]->width,
            frames[i]->height, 
            frames[i]->width*(frames[i]->bitsPerPixel/8), 
            frames[i]->bitsPerPixel, 
            0, 0, 0, true // in of006 this (topdown) had to be false.
        );	 
    
    
#ifdef TARGET_LITTLE_ENDIAN
        swapRgb(frames[i]);
#endif
        
        DWORD frameDuration = (DWORD) frames[i]->duration * 1000.f;
		
        // bmp =  FreeImage_ColorQuantize(bmp, FIQ_NNQUANT);
		
		// if we want to set a reduced color palette (2 to 256);
        bmp = FreeImage_ColorQuantizeEx(bmp, FIQ_NNQUANT, nColors);
		
		// dithering :)
		// you can set a different dither pattern for each frame
        // bmp = FreeImage_Dither(bmp, (FREE_IMAGE_DITHER)((i+1)%6));
        //bmp = FreeImage_Dither(bmp, FID_BAYER8x8);
        
		// clear any animation metadata used by this dib as we’ll adding our own ones 
		FreeImage_SetMetadata(FIMD_ANIMATION, bmp, NULL, NULL); 
		// add animation tags to dib[i] 
		FITAG *tag = FreeImage_CreateTag(); 
		
		if(tag) { 
			FreeImage_SetTagKey(tag, "FrameTime"); 
			FreeImage_SetTagType(tag, FIDT_LONG); 
			FreeImage_SetTagCount(tag, 1); 
			FreeImage_SetTagLength(tag, 4); 
			FreeImage_SetTagValue(tag, &frameDuration); 
			FreeImage_SetMetadata(FIMD_ANIMATION, bmp, FreeImage_GetTagKey(tag), tag); 
			FreeImage_DeleteTag(tag); 
		} 
		FreeImage_AppendPage(multi, bmp); 
	} 
	
	FreeImage_Unload(bmp);
	FreeImage_CloseMultiBitmap(multi); 
}
 
// from ofimage
//----------------------------------------------------
void ofxGifEncoder::swapRgb(ofxGifFrame * pix){
	if (pix->bitsPerPixel != 8){
		int sizePixels		= pix->width * pix->height;
		int cnt				= 0;
		unsigned char temp;
		int byteCount		= pix->bitsPerPixel/8;
        
		while (cnt < sizePixels){
			temp                            = pix->pixels[cnt*byteCount];
			pix->pixels[cnt*byteCount]		= pix->pixels[cnt*byteCount+2];
			pix->pixels[cnt*byteCount+2]	= temp;
			cnt++;
		}
	}
}

void ofxGifEncoder::exit() {
    stop();
}