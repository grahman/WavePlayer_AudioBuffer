#ifndef __GMBBUFFEROBJ__
#define	__GMBBUFFEROBJ__

#include "GMBUtil.h"

#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <Audioclient.h>
#include <Audiopolicy.h>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <exception>
#include <Windows.h>

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }


class GMBBufferObj;					//Forward declaration.

class CGMBSimpleObj;

enum  GMBAudioPlayerStateFlags			//Designed to fit into a single char variable
{
	kIdle = 0,
	kShouldPlay = 1,
	kShouldReverse = 2,
	kIsLoadingL = 4,
	kIsLoadingR = 8,
	kKill = 16
};

enum GMBAudioPlayerStateFlagBitPositions
{
	kShouldPlay_BitPos = 0,
	kShouldReverse_BitPos = 1,
	kIsLoadingL_BitPos = 2,
	kIsLoadingR_BitPos = 3,
	kKill_BitPos = 4,
};

struct GMBAudioRunLoopParams
{
	int flags;			//Please use GMBAudioPlayerStateFlags for setting and getting values here.
	float volL;			//Linear, fractional value please!
	float volR;			//Linear, fractional value please!
	float xfade;		//Cross fade position.
	float fltr_cu;
	float fltr_q;
};



struct GMBBufMarkers
{
	UINT32 Pos;
	UINT32 Beg;
	UINT32 End;
};

template<typename Type>
struct GMBStreamDescription
{
	bytes::GMBTypeArrayDescription<Type> samplesL;
	bytes::GMBTypeArrayDescription<Type> samplesR;
	bytes::GMBTypeArrayDescription<Type> samples;
};

struct GMBSystemAudioProperties
{
	HRESULT hr;// = S_OK;
	REFERENCE_TIME hnsRequestedDuration;// = REFTIMES_PER_SEC;
	REFERENCE_TIME hnsActualDuration;
	IMMDeviceEnumerator *pEnumerator;// = NULL;
	IMMDeviceCollection *pCollection;// = NULL;
	IMMDevice *pEndpoint;// = NULL;
	IPropertyStore *pProps;// = NULL;
	LPWSTR pwszID;// = NULL;
	IAudioClient *pAudioClient;// = NULL;
	IAudioRenderClient *pRenderClient;// = NULL;	//This is the thing from which we get the buffer.
	WAVEFORMATEX *pwfx;// = NULL;
	UINT32 bufferFrameCount;
	UINT32 numFramesAvailable;
	UINT32 numFramesPadding;
	BYTE *pData;// = NULL;							//This will be the buffer that we feed into pRenderClient's buffer.
	DWORD flags;// = 0;
	BYTE *myAudioSource;// = NULL;
	UINT32 numFramesWritten;
	UINT32 numBytesToRead;
	UINT32 numFloatsToRead;
	UINT32 numShortsToRead;
};


struct GMBBufferObjRef
{
	GMBBufferObj* pAddress;
	GMBAudioRunLoopParams* params;
	HANDLE ghMutex;
};

class GMBCircularBuffer;

class GMBBufferObj
{
public:

	GMBBufferObj();							//Default constructor
	GMBBufferObj(std::string _filename );	//Initialize with a filestream.
	GMBBufferObj(GMBBufferObjRef* bufferRef, HANDLE * mutexRef);
	//GMBBufferObj(const GMBBufferObj& src);	//Copy constructor.

	void init();

	DWORD WINAPI run(LPVOID _flags);										//Main run loop.
	void run(GMBAudioRunLoopParams* _flags);								//Single thread version.
	DWORD WINAPI GMBBufferObj::start(LPVOID _sharedAUPlayerParamsStruct);	//Another possible way to get into the main run loop

	void play();
	void pause();
	bool isPlaying();
	bool isReverse();
	bool isReadyL();
	bool isReadyR();

	//---------------Event related------------------------------------/
	void fire_posLChanged(ULONG x);
	

	//---------------Thread safe member funtions-----------------------/
	void requestStateChange(int _flag);
	void requestStateReset(int _flag);
	int getState()const;
	void getAudioDataL(float* DstArray, int  size);
	int getAudioDataSizeL();
	int getAudioDataSizeR();
	void loadAudioDataL(std::string path);
	void loadAudioDataR(std::string path);

	GMBAudioRunLoopParams getRLParams();								//Get a copy of the audio run loop parameters
	void setXFade(float _value);
	void setVolL(float _value);
	void setVolR(float _value);
	void setFltCU(float _value);
	void setFltQ(float _value);

	float getVolumeL();
	float getVolumeR();
	float getXFade();
	float getFltCU();
	float getFltQ();

	ULONG getPosL();

	/*void posChangedL_setCallback(CGMBSimpleObj instance);*/
	void posChangedR_setCallback(void(*inFn)(UINT));
	void posChangedR(UINT);
	
	//----------------End thread safe member functions------------------/

	//----------------Events------------------------------------------/
	void(*onPosChanged)(UINT);								//Event will be raised each cycle

	//-----------------End Events--------------------------------------/
	
	std::vector<std::string> getLog()const {return Log;};
	
	void test(){printf("%f%s", *(*TypeStream).samples.pArray);};
	


private:
	std::streampos getDataStartAddress();
	std::streampos getDataStartAddress(std::fstream *_file);
	
	void beginReading();
	//GMBBufferObjRef getBufferRef();


	bytes::GMBTypeArrayDescription<char>* Bytestream;
	std::streampos Beginning;
	char Filename[200];
	std::fstream* Filestream;
	size_t Size;
	GMBBufMarkers Markers;									//Holds information about the left stream including current postion.
	GMBBufMarkers MarkersR;									//Holds information about the right stream including current position.
	
	GMBSystemAudioProperties* SysProps;
	GMBStreamDescription<short>* Rawstream;
	GMBStreamDescription<short>* RawstreamR;
	GMBStreamDescription<float>* TypeStream;				//This is the float array that contains the entire waveform of the left audio player.
	GMBStreamDescription<float>* TypeStreamR;				//This is the float array that contains the entire waveform of the right audio player.

	GMBCircularBuffer* c_buf;								//The circular buffer that may have effects applied to it and that feeds into the soundcard.
	GMBStreamDescription<float> *processBufferL1;
	GMBStreamDescription<float> *processBufferR1;
	GMBStreamDescription<float> *processBuffer1;


	GMBStreamDescription<float> *processBufferL2;
	GMBStreamDescription<float> *processBufferR2;
	GMBStreamDescription<float> *processBuffer2;
	 
	

	//Important state variable---------------------------
	GMBAudioRunLoopParams * rlParams;

	std::thread t1;
	HANDLE * ghMutex;

	//Other
	std::exception Ex;
	std::vector<std::string> Log;


	//Events--------------------------------------------
	long (CGMBSimpleObj::*__OnPosLChangedFnPtr)(ULONG);			//Pointer to the function that will handle the updating of the waveform view.
	long (*__OnPosRChangedFnPtr)(ULONG);			//Pointer to the function that will handle the updating of the waveform view.

	

	//End Events----------------------------------------

	//Helpful booleans and other stuff
	bool AudioDataReadyL;
	bool AudioDataReadyR;

};

class GMBCircularBuffer
{
public:
	GMBCircularBuffer(int MAX_SIZE);			//Default constructor
	
	/******************Mutator Functions**************************************/
	void write(float* input, int nElementsToWrite);
	void write_reverse(bytes::GMBTypeArrayDescription<float>* _in);

	void write_mux(bytes::GMBTypeArrayDescription<float>* inputL, float gainL, bytes::GMBTypeArrayDescription<float>* inputR, float gainR);
	//void write_mux_reverse(float* inputL, float gainL, float* inputR, float gainR, int nElementsToWrite);
	float& operator[](int i);
	
	/*****************Accessor Functions***************************************/
	void read(float* dst, int nElementsToRead);

private:
	int pos;
	int r_pos;										//"read" position (cursor)
	int ARR_LENGTH;
	float* arr;										//The underlying array

};

/**********************Non member function************************/


/**********************DSP Functions******************************/
void GMBProcessArray_GainCoeffL(float* src, float coeff, int count);
void GMBProcessArray_BiQuad2ndOrderLPF(float* src, float fc, float Q, int N);
void GMBInitFilterConstants();
void DeinterleaveFloatArray(float* l, float* r, float* src, int src_length);		//Expects l and r to be initilialized with a length of >= src_length / 2
void InterleaveFloatArray(float* l, float* r, float* dst, int dst_length);

#endif
