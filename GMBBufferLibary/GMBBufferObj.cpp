//#ifndef __GMBBUFFEROBJTEMPIMP__
//#define __GMBBUFFEROBJTEMPIMP__

#include "GMBBufferObj.h"
#include "GMBUtil.h"
#include <iostream>
#include <thread>
#include <exception>
#include <WinBase.h>
#include <cmath>
#include <Windows.h>
//#include "GMBSimpleObj.h"

using namespace std;
using namespace bytes;
using namespace misc;

const float  PI_F=3.14159265358979f;

//Filter coefficients global vars, etc...
	float fc_r;				//Angular cutoff frequency
	float d;
	float beta;
	float gamma;
	float a0;
	float a1;
	float a2;
	float b1;
	float b2;

	float xnL;
	float xn_1L;
	float xn_2L;

	float xnR;
	float xn_1R;
	float xn_2R;

	float ynL;
	float yn_1L;
	float yn_2L;
	
	float ynR;
	float yn_1R;
	float yn_2R;

HANDLE ghMutex;
std::string LOGPATH = "C:\\Users\\Graham\\Programming\\Logs\\loadAudioDataLog.txt";
std::string LOGPATH_START = "C:\\Users\\Graham\\Programming\\Logs\\startLog.txt";
std::string LOGPATH_BEGINREADING = "C:\\Users\\Graham\\Programming\\Logs\\beginReadingLog.txt";
std::string LOGPATH_INIT = "C:\\Users\\Graham\\Programming\\Logs\\initLog.txt";
std::string LOGPATH_DSPFILTER = "C:\\Users\\Graham\\Programming\\Logs\\dspFilterLog.txt";
std::string LOGPATH_EVENTS = "C:\\Users\\Graham\\Programming\\Logs\\interopEventsLog.txt";

GMBBufferObj::GMBBufferObj()
{
	this->init();
}

GMBBufferObj::GMBBufferObj(string _filename)
{
	//return;
	Log.push_back("Beginning Construction...");
	cout << "Beginning Construction...";
	int length = 0;
	int data_size = 0;
	int short_data_size = 0;
	const char * str;

	

	str = _filename .c_str();
	length = _filename.length() + 1;

	strcpy_s(this->Filename, _filename.length() + 1, _filename.c_str() );
	
	cout << "Supplied filename is: ";
	printf("%c", *str);
	cout << "Opening the file\n";
	
	
}

GMBBufferObj::GMBBufferObj(GMBBufferObjRef* bufferRef, HANDLE * mutexRef)
{
	//return;
	//cout << "Beginning Construction...";
	onPosChanged = NULL;
	__OnPosLChangedFnPtr = NULL;
	__OnPosRChangedFnPtr = NULL;
	this->rlParams = new GMBAudioRunLoopParams();
	this->ghMutex = mutexRef;
	bufferRef->pAddress = this;
	bufferRef->params = this->rlParams; 
	TypeStream = new GMBStreamDescription<float>();
	TypeStreamR = new GMBStreamDescription<float>();
	Bytestream = new GMBTypeArrayDescription<char>();
	Rawstream = new GMBStreamDescription<short>();
	RawstreamR = new GMBStreamDescription<short>();

	TypeStream->samples.pArray = NULL;
	TypeStream->samplesL.pArray = NULL;
	TypeStream->samplesR.pArray = NULL;
	TypeStreamR->samples.pArray = NULL;
	TypeStreamR->samplesL.pArray = NULL;
	TypeStreamR->samplesR.pArray = NULL;
	Rawstream->samples.pArray = NULL;
	Rawstream->samplesL.pArray = NULL;
	Rawstream->samplesR.pArray = NULL;
	RawstreamR->samples.pArray = NULL;
	RawstreamR->samplesL.pArray = NULL;
	RawstreamR->samplesR.pArray = NULL;

	//c_buf = new GMBCircularBuffer(2048);
	processBufferL1 = new GMBStreamDescription<float>();
	processBufferR1 = new GMBStreamDescription<float>();
	processBuffer1 = new GMBStreamDescription<float>();

	processBufferL2 = new GMBStreamDescription<float>();
	processBufferR2 = new GMBStreamDescription<float>();
	processBuffer2 = new GMBStreamDescription<float>();


	processBufferL1->samples.nElements = 2048;
	processBufferR1->samples.nElements = 2048;
	processBuffer1->samples.nElements = 4096;

	processBufferL2->samples.nElements = 2048;
	processBufferR2->samples.nElements = 2048;
	processBuffer2->samples.nElements = 4096;

	processBufferL1->samples.pArray = new float[4096];
	processBufferR1->samples.pArray = new float[4096];
	processBuffer1->samples.pArray = new float[4096 * 2];

	processBufferL2->samples.pArray = new float[4096];
	processBufferR2->samples.pArray = new float[4096];
	processBuffer2->samples.pArray = new float[4096 * 2];

	AudioDataReadyL = false;
	AudioDataReadyR = false;

}

//GMBBufferObj::GMBBufferObj(const GMBBufferObj& SrcObj)
//{
//	return;
//}

DWORD WINAPI GMBBufferObj::run(LPVOID _sharedAUPlayerParamsStruct)
{
	this->rlParams = (GMBAudioRunLoopParams*)_sharedAUPlayerParamsStruct;
	int length = 0;
	int data_size = 0;
	int short_data_size = 0;
	ofstream output_log("log.txt", ios::out);

	/*shouldstop = ref->pShouldStop;
	reversed = new bool(false);
	playing = ref->pIsPlaying; 
	shouldplay  = ref->pShouldPlay;
	kill  = ref->pKill;*/

	//BufferState = new int(0);

	//this->Filestream = new fstream("C:\\Users\\Graham\\Documents\\Visual Studio 2012\\Projects\\ConsoleApplication4\\Debug\\Content\\ott.wav", ios::ate | ios::in | ios::binary);
	this->Filestream = new fstream("C:\\Users\\Graham\\Documents\\Visual Studio 2012\\Projects\\ConsoleApplication4\\Debug\\Content\\ott.wav", ios::ate | ios::in | ios::binary);
	
	if (Filestream->fail()){
		cout << "Opening the stream from the constructor failed\n";
		return 1;
	}
	length = this->Filestream->tellg();				//Gets the total length of stream (in bytes)
	this->Beginning = getDataStartAddress();
	this->Filestream->seekg(this->Beginning);
	data_size = length - (int)this->Beginning;

	//Take this oppurtunity to init our instance markers
	Markers.Beg = 0;
	Markers.Pos = 0;
	Markers.End = data_size / 2;

	output_log << "Markers.End = " << Markers.End << endl;

	this->Bytestream = new GMBTypeArrayDescription<char>();
	this->Rawstream = new GMBStreamDescription<short>();
	this->TypeStream = new GMBStreamDescription<float>();

	this->Bytestream->nElements = data_size;
	this->Bytestream->pArray = new char[data_size];
	this->Filestream->read(this->Bytestream->pArray, data_size);

	output_log << "Bytestream->nElements = " << data_size << " bytessss" << endl;
	
	output_log << "Creating arrays...\n";
	this->Rawstream->samples.pArray = new short[this->Bytestream->nElements / 2];
	this->Rawstream->samples.nElements = this->Bytestream->nElements / 2;

	GMBCastBytesToTypeArray_shortVersion<short>(&(this->Rawstream->samples), (this->Bytestream), 1);
	output_log << "Arrays created...\n";
	

	//Now deinterleave the arrays so we have the left channel and right channel separated.
	GMBDeinterleaveArrays<short>(&this->Rawstream->samples, &this->Rawstream->samplesL, &this->Rawstream->samplesR);
	GMBCastBytesToTypeArray<float, short>(&(this->TypeStream->samples), Bytestream, 1);

	//Delete[] byteStream
	if (Bytestream->pArray != NULL)
	{
		delete[] Bytestream->pArray;
		Bytestream->pArray = NULL;
	}

	/***********************In this section, normalize the float array**********************************/
		for (int i=0; i < TypeStream->samples.nElements; ++i){
			*(TypeStream->samples.pArray + i) = *(TypeStream->samples.pArray + i) / MAXSHORT;
		}

	output_log << "About to call init()...\n";
	output_log.close();
	this->init();

	return 0;
}

void GMBBufferObj::init()
{

	ofstream olog(LOGPATH_INIT, ios::app);
	olog << "Init started...\n";

	olog << "About to call init()...\n";
	olog.close();
	
	//BufferState = new int(0);
	//Log.push_back("Initializing");
	size_t sizeofframe = 2 * sizeof(float);
	SysProps = new GMBSystemAudioProperties();
	//----Initialize all the system properties.
	this->SysProps->hr = S_OK;
	//this->SysProps->hnsRequestedDuration = REFTIMES_PER_SEC;
	this->SysProps->hnsRequestedDuration = REFTIMES_PER_SEC * 0.2;			//Shorter buffer length for more responsiveness.
	this->SysProps->pEnumerator = NULL;
	this->SysProps->pCollection = NULL;
	this->SysProps->pEndpoint = NULL;
	this->SysProps->pProps = NULL;
	this->SysProps->pwszID = NULL;
	this->SysProps->pAudioClient = NULL;
	this->SysProps->pRenderClient = NULL;
	this->SysProps->pwfx = NULL;
	this->SysProps->pData = NULL;
	this->SysProps->flags = 0;
	this->SysProps->myAudioSource = NULL;

	CoInitialize(NULL);

	SysProps->hr = CoCreateInstance(
	CLSID_MMDeviceEnumerator, NULL,
	CLSCTX_ALL, IID_IMMDeviceEnumerator,
	(void**)&(this->SysProps->pEnumerator) );

	//CHECK_HR(SysProps->hr, "CoCreateInstance deviceenumerator");

	SysProps->hr = SysProps->pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &(this->SysProps->pEndpoint));
	//CHECK_HR(SysProps->hr, "GetDefaultAudioEndpoint");

	PROPVARIANT varName;
	PropVariantInit(&varName);

	SysProps->hr = SysProps->pEndpoint->OpenPropertyStore(STGM_READ, &(this->SysProps->pProps));
	//CHECK_HR(SysProps->hr, "OpenPropertyStore");

	PROPERTYKEY pkey = PKEY_Device_FriendlyName;
	SysProps->hr = SysProps->pProps->GetValue(pkey, &varName);

	//CHECK_HR(SysProps->hr, "Get property values");

	SysProps->hr = SysProps->pEndpoint->Activate(IID_IAudioClient, CLSCTX_ALL,
								NULL, (void**)&(this->SysProps->pAudioClient));
	//CHECK_HR(SysProps->hr, "pendpoint activate");

	SysProps->hr = this->SysProps->pAudioClient->GetMixFormat(&(this->SysProps->pwfx));
	//CHECK_HR(SysProps->hr, "GetMixFormat");

	//Initialize the audio client next
	SysProps->hr = this->SysProps->pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
									0,
									this->SysProps->hnsRequestedDuration,
									0,
									SysProps->pwfx,
									NULL);
	//CHECK_HR(SysProps->hr, "pAudioClient Initialize");

	//Tell the audio source which format to use.
	SysProps->hr = this->SysProps->pAudioClient->GetBufferSize(&(this->SysProps->bufferFrameCount));
	//CHECK_HR(SysProps->hr, "pAudioClient get buffer size");

	SysProps->hr = this->SysProps->pAudioClient->GetService(IID_IAudioRenderClient, (void**)&(this->SysProps->pRenderClient));
	//CHECK_HR(SysProps->hr, "pAudioClient Get Service");

	SysProps->hr = this->SysProps->pRenderClient->GetBuffer(this->SysProps->bufferFrameCount, &(SysProps->pData));						//Here, we fill the entire buffer with the initial data before we start playing.
	//CHECK_HR(SysProps->hr, "Get Buffer (first time)");

	//memcpy(this->SysProps->pData, TypeStream->samples.pArray, SysProps->bufferFrameCount * sizeof(float) * 2);
	Markers.Pos += SysProps->bufferFrameCount * 2;
	SysProps->hr = SysProps->pRenderClient->ReleaseBuffer(SysProps->bufferFrameCount, SysProps->flags);
	//CHECK_HR(SysProps->hr, "Release Buffer (first time)");


	this->SysProps->hnsActualDuration = (double)REFTIMES_PER_SEC * SysProps->bufferFrameCount / SysProps->pwfx->nSamplesPerSec;

	//cout << "Init() was successful\nNow calling start()...";
	//this->start();

}


streampos GMBBufferObj::getDataStartAddress()
{
	//assert(Filestream.is_open());
	/*if (!Filestream.is_open()){
		Ex = *(new exception("getStartAddressError: Filestream was not open"));
		throw Ex;
	}*/
	bool fileWasOpen = true;
	if (!Filestream->is_open())
	{
		fileWasOpen = false;
		this->Filestream->open(this->Filename, ios::beg | ios::in | ios::binary);
	}

	//-----Variables------
	char tag[5];
	char match[] = "data";
	int cmpValue = 1;
	int frame_size = 4;												//Frame size is number of channels * bit rate / 8. We are using 4 bytes per frame because we know the file being used.
	int wave_data_pos = 0;
	this->Filestream->seekg(0, ios::beg);									//Rewind wave file back to beginning.
	streampos temp;
		while (true)
		{
			this->Filestream->read(tag, 4);
			*(tag + 4) = '\0';											//Adding the terminating null character, otherwise strcmp doesn't work properly.
			cmpValue = strcmp(tag, match);
		/*	for (int i=0; i < 4; ++i)
			{
				cout << (char)*(tag + i);
			}*/
			//cout << endl;
			if (cmpValue == 0)											//strcmp returns 0 if there is a match.
			{
				break;
			}
		}

		
		temp = Filestream->tellg();
		if (!fileWasOpen)
		{
			Filestream->close();											//We close the file only if it was closed before function was called.
		}
		//cout << "Got start Address\n";

		return temp;
}

streampos GMBBufferObj::getDataStartAddress(fstream* _file)
{
	//bool fileWasOpen = true;
	/*if (!_file->is_open())
	{
		fileWasOpen = false;
		_file->open(this->Filename, ios::beg | ios::in | ios::binary);
	}*/

	//-----Variables------
	ofstream olog;
	olog.open(LOGPATH, ios::app);
	olog << "GMBBufferObj::getDataStartAddress says: Hello!\n";

	olog << "GMBBufferObj::getDataStartAddress says: _file fags are: " << _file->rdstate() << "\n";
	char tag[5];
	char match[] = "data";
	int cmpValue = 1;
	int frame_size = 4;												//Frame size is number of channels * bit rate / 8. We are using 4 bytes per frame because we know the file being used.
	int wave_data_pos = 0;
	//----End Variables----
	_file->seekg(0, ios::beg);										//Rewind wave file back to beginning.
	streampos temp;
	while (true)
	{
		_file->read(tag, 4);
		*(tag + 4) = '\0';											//Adding the terminating null character, otherwise strcmp doesn't work properly.
		cmpValue = strcmp(tag, match);

		if (cmpValue == 0)											//strcmp returns 0 if there is a match.
		{
			break;
		}
	}

	temp = _file->tellg();
	olog << (int)temp << endl;
	olog.close();
	return temp;
}


DWORD WINAPI GMBBufferObj::start(LPVOID _sharedAUPlayerParamsStruct)
{
	ofstream olog(LOGPATH_START, ios::app);
	olog << "GMBBufferObj::start says: Hello!\n";
	olog << "Currently, state is " << getState() << endl;
	
	this->requestStateReset(kShouldPlay);
	int flags = getState();

	olog << "Now, state is " << flags << endl;
	olog.close();
	while (flags < kKill)
	{
		if (GMBInspectBit(flags, kShouldPlay_BitPos))		//Check if kShouldPlay flag is set
		{
			this->beginReading();
		}
		flags = getState();
	}
	SysProps->pAudioClient->Stop();
	olog.open(LOGPATH_START, ios::app);
	olog << "GMBBufferObj::start is returning now\n";
	olog.close();
	return 0;
}

void GMBBufferObj::play()
{
	GMBAudioRunLoopParams rl;
	rl = getRLParams();
	if (GMBInspectBit(rl.flags, kShouldPlay))
	{
		return;				//If already in a playback state, return without doing anything.
	}
	requestStateChange(kShouldPlay_BitPos);
}

void GMBBufferObj::pause()
{
	GMBAudioRunLoopParams rl;
	rl = getRLParams();
	if (!GMBInspectBit(rl.flags, kShouldPlay))
	{
		return;				//If already in a playback state, return without doing anything.
	}
	requestStateChange(kShouldPlay_BitPos);
}

bool GMBBufferObj::isPlaying()
{
	GMBAudioRunLoopParams rl;
	rl = getRLParams();
	if (GMBInspectBit(rl.flags, kShouldPlay))
	{
		return true;
	}
	else
	{
		return false;
	}
	
}

void GMBBufferObj::requestStateChange(int _flagBitPosition)
{
	if (ghMutex == NULL || rlParams == NULL)
	{
		cout << "The problem is in requestStateChange\n";
		system("PAUSE");
		assert(0);
	}
	WaitForSingleObject(ghMutex, INFINITE);
	
	rlParams->flags = GMBInvertBit(rlParams->flags, _flagBitPosition);
	ReleaseMutex(ghMutex);
	return;
}

void GMBBufferObj::requestStateReset(int _flag)
{
	WaitForSingleObject(ghMutex, INFINITE);

	rlParams->flags = _flag;

	ReleaseMutex(ghMutex);
	return;
}

int GMBBufferObj::getState()const
{
	if (ghMutex == NULL || rlParams == NULL)
	{
		cout << "Mutex or Params not set!\n";
		assert(0);
	}

	int flagsCopy;
	WaitForSingleObject(ghMutex, INFINITE);
	flagsCopy = rlParams->flags;
	ReleaseMutex(ghMutex);
	return flagsCopy;
}

GMBAudioRunLoopParams GMBBufferObj::getRLParams()
{
	GMBAudioRunLoopParams outParams;
	WaitForSingleObject(ghMutex, INFINITE);			//Wait for mutex object to become available

	outParams = *this->rlParams;

	ReleaseMutex(ghMutex);
	return outParams;
}

//Below is the original beginReading() function. Still works great, but doesn't do special effects.

//void GMBBufferObj::beginReading()
//{
//
//	HRESULT hr;
//	int state;
//	ofstream olog("log.txt", ios::app);
//	
//	try
//	{
//		hr = SysProps->pAudioClient->Start();
//		//CHECK_HR(hr, "pAudioClient start");
//	} catch (exception e)
//	{
//		/*printf("beginRead() error: ");
//		printf(e.what());
//		cout << "\n";*/
//	}
//
//	
//		
//	SysProps->numFramesAvailable = SysProps->bufferFrameCount;
//	int debugCounter = 0;
//
//	//Markers.Pos = 0;
//	//cout << "Here is the while loop.\n";
//	state = getState();
//
//	olog << "Before entering loop, Markers.Pos = " << Markers.Pos << endl;
//	olog << "Markers.Beg = " << Markers.Beg;
//	while(GMBInspectBit(state, kShouldPlay_BitPos))
//	{
//		Sleep((DWORD)(SysProps->hnsActualDuration/REFTIMES_PER_MILLISEC/2));
//		hr = SysProps->pAudioClient->GetCurrentPadding(&(SysProps->numFramesPadding));
//		LOG_HR(hr, "Get Current Padding", olog);
//
//		olog << "Padding is now: " << SysProps->numFramesPadding << endl;
//		SysProps->numFramesAvailable = SysProps->bufferFrameCount - SysProps->numFramesPadding;
//		debugCounter++;
//		olog << debugCounter << endl;
//		olog << "Markers.Pos = " << Markers.Pos << endl;
//		if (Markers.Pos + (SysProps->numFramesAvailable * 2) > TypeStream->samples.nElements)
//		{
//			SysProps->numFloatsToRead = (TypeStream->samples.nElements - Markers.Pos);
//			SysProps->numFramesAvailable = SysProps->numFloatsToRead / 2;
//			Markers.Pos = 0;
//		}
//		else 
//		{
//			olog << Markers.Pos << " + " << SysProps->numFramesAvailable << " <= " << TypeStream->samples.nElements << endl;
//			olog << "Therefore, we are making 'numFloatsToRead' = numFramesAvailable * 2 which is " << SysProps->numFramesAvailable * 2 << endl;
//			SysProps->numFloatsToRead = SysProps->numFramesAvailable * 2;
//			/*Markers.Pos += (SysProps->numFloatsToRead);*/
//		}
//		olog << "Number of frames available is now: " << SysProps->numFramesAvailable << endl;
//		hr = SysProps->pRenderClient->GetBuffer(SysProps->numFramesAvailable, &(SysProps->pData));
//		LOG_HR(hr, "GetBuffer", olog);
//
//		//*******Load the shared buffer with the initial data, fill in later****
//
//		try 
//		{
//			olog << "Memcopying float " << Markers.Pos << " to float " << Markers.Pos + SysProps->numFloatsToRead << ", which out of ";
//			olog << TypeStream->samples.nElements << " elements is " << ((float)Markers.Pos + SysProps->numFloatsToRead / (float)TypeStream->samples.nElements) << "% of the buffer\n";
//			memcpy(SysProps->pData, (TypeStream->samples.pArray + Markers.Pos), (SysProps->numFloatsToRead * sizeof(float)));
//			Markers.Pos += (SysProps->numFloatsToRead);
//		}
//		catch (exception ex)
//		{
//			olog << "Exception thrown: " << ex.what() << endl;
//			assert(true);
//		}
//		
//		
//		/*if (Markers.Pos >= Markers.End)
//		{
//			Markers.Pos = Markers.Beg;
//			olog << "Just made Markers.Pos = " << Markers.Beg;
//		}*/
//
//			
//
//		//olog << "Markers.Pos = " << Markers.Pos << endl;
//
//		//**********************************************************************
//
//		SysProps->hr = SysProps->pRenderClient->ReleaseBuffer(SysProps->numFramesAvailable, SysProps->flags);
//		LOG_HR(SysProps->hr, "Release Buffer", olog);
//		state = getState();
//	}
//		//Sleep((DWORD)(SysProps->hnsActualDuration/REFTIMES_PER_MILLISEC/2));
//		SysProps->pAudioClient->Stop();
//}

//End original beginReading() function

void GMBBufferObj::beginReading()
{

	HRESULT hr;
	int state;									//Local copy of the getState() variable.
	GMBAudioRunLoopParams rl_local;				//Local copy of the run loop parameters.
	ofstream olog(LOGPATH_BEGINREADING, ios::app);
	
	try
	{
		hr = SysProps->pAudioClient->Start();
	} catch (exception e)
	{
	
	}

	SysProps->numFramesAvailable = SysProps->bufferFrameCount;
	int debugCounter = 0;

	//Markers.Pos = 0;
	//cout << "Here is the while loop.\n";
	state = getState();

	olog << "Before entering loop, Markers.Pos = " << Markers.Pos << endl;
	olog << "Markers.Beg = " << Markers.Beg << endl;
	olog << "State = " << state << endl;
	olog.close();
	while(GMBInspectBit(state, kShouldPlay_BitPos))
	{
		Sleep((DWORD)(SysProps->hnsActualDuration/REFTIMES_PER_MILLISEC/2));
		//Obtain a local copy of the rlParams
		rl_local = getRLParams();
		switch(state)
		{
			case 1:				//kShouldPlay
				hr = SysProps->pAudioClient->GetCurrentPadding(&(SysProps->numFramesPadding));
				LOG_HR(hr, "Get Current Padding", olog);

				olog << "Padding is now: " << SysProps->numFramesPadding << endl;
				SysProps->numFramesAvailable = SysProps->bufferFrameCount - SysProps->numFramesPadding;
				debugCounter++;
				olog << debugCounter << endl;
				olog << "Markers.Pos = " << Markers.Pos << endl;
				if (Markers.Pos + (SysProps->numFramesAvailable * 2) > TypeStream->samples.nElements)
				{
					SysProps->numFloatsToRead = (TypeStream->samples.nElements - Markers.Pos);
					SysProps->numFramesAvailable = SysProps->numFloatsToRead / 2;
					Markers.Pos = 0;
				}
				else 
				{
					olog << Markers.Pos << " + " << SysProps->numFramesAvailable << " <= " << TypeStream->samples.nElements << endl;
					olog << "Therefore, we are making 'numFloatsToRead' = numFramesAvailable * 2 which is " << SysProps->numFramesAvailable * 2 << endl;
					SysProps->numFloatsToRead = SysProps->numFramesAvailable * 2;
					/*Markers.Pos += (SysProps->numFloatsToRead);*/
				}
				olog << "Number of frames available is now: " << SysProps->numFramesAvailable << endl;
				hr = SysProps->pRenderClient->GetBuffer(SysProps->numFramesAvailable, &(SysProps->pData));
				LOG_HR(hr, "GetBuffer", olog);

				//*******Load the shared buffer with the initial data, fill in later****

				try 
				{
					olog << "Memcopying float " << Markers.Pos << " to float " << Markers.Pos + SysProps->numFloatsToRead << ", which out of ";
					olog << TypeStream->samples.nElements << " elements is " << ((float)Markers.Pos + SysProps->numFloatsToRead / (float)TypeStream->samples.nElements) << "% of the buffer\n";
					memcpy(processBuffer1->samples.pArray, TypeStream->samples.pArray + Markers.Pos, (SysProps->numFloatsToRead * sizeof(float)));				//Copy a little more of song into process array.
					memcpy(SysProps->pData, TypeStream->samples.pArray + Markers.Pos, (SysProps->numFloatsToRead * sizeof(float)));
					GMBProcessArray_GainCoeffL((float*)SysProps->pData, rl_local.volL, SysProps->numFloatsToRead);
					GMBProcessArray_BiQuad2ndOrderLPF((float*)SysProps->pData, rl_local.fltr_cu, rl_local.fltr_q, SysProps->numFloatsToRead);

					
					Markers.Pos += (SysProps->numFloatsToRead);
				}
				catch (exception ex)
				{
					olog << "Exception thrown: " << ex.what() << endl;
					assert(true);
				}
		
		
				/*if (Markers.Pos >= Markers.End)
				{
					Markers.Pos = Markers.Beg;
					olog << "Just made Markers.Pos = " << Markers.Beg;
				}*/

			

				//olog << "Markers.Pos = " << Markers.Pos << endl;

				//**********************************************************************
				fire_posLChanged((ULONG)Markers.Pos);
				SysProps->hr = SysProps->pRenderClient->ReleaseBuffer(SysProps->numFramesAvailable, SysProps->flags);
				LOG_HR(SysProps->hr, "Release Buffer", olog);
				break;
			case 3:				//kShouldReverse
				break;			
			case 5:				//kShouldPlay, kIsLoadingL
				break;
			case 7:				//kShouldPlay, kIsLoadingL, kShouldReverse
				break;
			case 9:				//kShouldPlay, kIsLoadingR
				break;
			case 11:			//kShouldPlay, kIsLoadingR, kShouldReverse
				break;
		}
		state = getState();
	}	
}

//GMBBufferObjRef GMBBufferObj::getBufferRef() 
//{
//	GMBBufferObjRef ref();
//	
//	return ref;
//}

void GMBBufferObj::getAudioDataL(float* dst, int size)
{
	size = TypeStream->samples.nElements;
	for (int i=0; i < size; ++i)
	{
		*(dst + i) = *(TypeStream->samples.pArray + i);
	}
}

int GMBBufferObj::getAudioDataSizeL()
{
	if (TypeStream->samples.pArray == NULL)
		return 0;
	int size = 0;
	WaitForSingleObject(ghMutex, INFINITE);

	size = TypeStream->samples.nElements;

	ReleaseMutex(ghMutex);

	return size;
}

int GMBBufferObj::getAudioDataSizeR()
{
	if (TypeStreamR->samples.pArray == NULL)
		return 0;
	int size = 0;
	WaitForSingleObject(ghMutex, INFINITE);

	size = TypeStreamR->samples.nElements;

	ReleaseMutex(ghMutex);

	return size;
}

bool GMBBufferObj::isReadyL()
{
	bool result;
	WaitForSingleObject(ghMutex, INFINITE);
	result = AudioDataReadyL;
	ReleaseMutex(ghMutex);
	return result;
}

bool GMBBufferObj::isReadyR()
{
	bool result;
	WaitForSingleObject(ghMutex, INFINITE);
	result = AudioDataReadyR;
	ReleaseMutex(ghMutex);
	return result;
}

void GMBBufferObj::loadAudioDataL(string path)
{
	///*-----------Variables----------------*/
	std::fstream *f = new fstream();						//The file we want to load
	std::ofstream *olog = new ofstream();
	streampos dataStartPos;			//The start pos of the audio data in the wave file.
	int dataEndPos;
	int dataLength;
	int originalState = getState();

	///*-----------End Variables------------*/
	olog->open("C:\\Users\\Graham\\Programming\\Logs\\loadAudioDataLog.txt", ios::out);
	*olog << "GMBBufferObj::loadAudioDataL says \n\t'Supplied string is: " << path << "'\n";
	

	//*olog << "Attempting to load data from " << path << endl;
	*olog << "Opening the audio file...\n";
	f->open(path, ios::ate | ios::in | ios::binary);
	if (f->fail())
	{
		*olog << "Opening the audio file failed!\n";
	}

	dataEndPos = (int)f->tellg();
	/**olog << "Opened successfully...\n";*/
	requestStateChange(originalState | kIsLoadingL);
	Sleep(500);						//Sleep to give the main run loop enough time to find out we are loading another file before we start loading.
	olog->close();						//Close because function on next line needs olog

	dataStartPos = getDataStartAddress(f);

	olog->open(LOGPATH, ios::app);
	*olog << "dataEndPos: " << (int)dataEndPos << "\ndataStartPos: " << (int)dataStartPos << "\n";
	*olog << "dataLength will be: " << (int)(dataEndPos - dataStartPos) << "\n";
	dataLength = dataEndPos - dataStartPos;		//Now we have the size of the new array buffer.

	//Delete[] all the previous data
	if (TypeStream->samples.pArray != NULL)
	{
		delete[] TypeStream->samples.pArray;
		TypeStream->samples.pArray = NULL;
	}
	if (TypeStream->samplesL.pArray != NULL)
	{
		delete[] TypeStream->samplesL.pArray;
		TypeStream->samplesL.pArray = NULL;
	}

	if (TypeStream->samplesR.pArray != NULL)
	{
		delete[] TypeStream->samplesR.pArray;
		TypeStream->samplesR.pArray = NULL;
	}


	//Now initialize all the new stuff
	Bytestream->nElements = dataLength;
	Bytestream->pArray = new char[dataLength];
	f->read(Bytestream->pArray, dataLength);

	Rawstream->samples.nElements = dataLength / sizeof(short);
	GMBCastBytesToTypeArray_shortVersion<short>(&(this->Rawstream->samples), (this->Bytestream), 1);

	//Now deinterleave the arrays so we have the left channel and right channel separated.
	GMBDeinterleaveArrays<short>(&this->Rawstream->samples, &this->Rawstream->samplesL, &this->Rawstream->samplesR);
	GMBCastBytesToTypeArray<float, short>(&(this->TypeStream->samples), Bytestream, 1);

	//Set the markers
	Markers.Beg = 0;
	Markers.Pos = 0;
	Markers.End = dataLength / 2;

	//Delete[] byteStream
	if (Bytestream->pArray != NULL)
	{
		delete[] Bytestream->pArray;
		Bytestream->pArray = NULL;
	}

	///***********************In this section, normalize the float array**********************************/
		for (int i=0; i < TypeStream->samples.nElements; ++i){
			*(TypeStream->samples.pArray + i) = *(TypeStream->samples.pArray + i) / MAXSHORT;
		}

	////Also close the file.
	f->close();
	delete f;
	f = NULL;

	*olog << "Load data successful!\n";
	*olog << "\toriginalState was " << originalState << endl;
	olog->close();
	delete olog;
	olog = NULL;
	GMBInitFilterConstants();
	//Now that the buffer is filled, we can reset the state
	requestStateReset(originalState);
	AudioDataReadyL = true;
}

void GMBBufferObj::loadAudioDataR(string path)
{
///*-----------Variables----------------*/
	std::fstream *f = new fstream();						//The file we want to load
	std::ofstream *olog = new ofstream();
	streampos dataStartPos;			//The start pos of the audio data in the wave file.
	int dataEndPos;
	int dataLength;
	int originalState = getState();

	///*-----------End Variables------------*/
	olog->open("C:\\Users\\Graham\\Programming\\Logs\\loadAudioDataLog.txt", ios::out);
	*olog << "GMBBufferObj::loadAudioDataL says \n\t'Supplied string is: " << path << "'\n";
	

	//*olog << "Attempting to load data from " << path << endl;
	*olog << "Opening the audio file...\n";
	f->open(path, ios::ate | ios::in | ios::binary);
	if (f->fail())
	{
		*olog << "Opening the audio file failed!\n";
	}

	dataEndPos = (int)f->tellg();
	/**olog << "Opened successfully...\n";*/
	requestStateChange(originalState | kIsLoadingR);
	Sleep(500);						//Sleep to give the main run loop enough time to find out we are loading another file before we start loading.
	olog->close();						//Close because function on next line needs olog

	dataStartPos = getDataStartAddress(f);

	olog->open(LOGPATH, ios::app);
	*olog << "dataEndPos: " << (int)dataEndPos << "\ndataStartPos: " << (int)dataStartPos << "\n";
	*olog << "dataLength will be: " << (int)(dataEndPos - dataStartPos) << "\n";
	dataLength = dataEndPos - dataStartPos;		//Now we have the size of the new array buffer.

	//Delete[] all the previous data
	if (TypeStreamR->samples.pArray != NULL)
	{
		delete[] TypeStreamR->samples.pArray;
		TypeStreamR->samples.pArray = NULL;
	}
	if (TypeStreamR->samplesL.pArray != NULL)
	{
		delete[] TypeStreamR->samplesL.pArray;
		TypeStreamR->samplesL.pArray = NULL;
	}

	if (TypeStreamR->samplesR.pArray != NULL)
	{
		delete[] TypeStreamR->samplesR.pArray;
		TypeStreamR->samplesR.pArray = NULL;
	}


	//Now initialize all the new stuff
	Bytestream->nElements = dataLength;
	Bytestream->pArray = new char[dataLength];
	f->read(Bytestream->pArray, dataLength);

	Rawstream->samples.nElements = dataLength / sizeof(short);
	GMBCastBytesToTypeArray_shortVersion<short>(&(this->Rawstream->samples), (this->Bytestream), 1);

	//Now deinterleave the arrays so we have the left channel and right channel separated.
	GMBDeinterleaveArrays<short>(&this->Rawstream->samples, &this->Rawstream->samplesL, &this->Rawstream->samplesR);
	GMBCastBytesToTypeArray<float, short>(&(this->TypeStreamR->samples), Bytestream, 1);

	//Set the markers
	Markers.Beg = 0;
	Markers.Pos = 0;
	Markers.End = dataLength / 2;

	//Delete[] byteStream
	if (Bytestream->pArray != NULL)
	{
		delete[] Bytestream->pArray;
		Bytestream->pArray = NULL;
	}

	///***********************In this section, normalize the float array**********************************/
		for (int i=0; i < TypeStreamR->samples.nElements; ++i){
			*(TypeStreamR->samples.pArray + i) = *(TypeStreamR->samples.pArray + i) / MAXSHORT;
		}

	////Also close the file.
	f->close();
	delete f;
	f = NULL;

	*olog << "Load data successful!\n";
	olog->close();
	delete olog;
	olog = NULL;
	//Now that the buffer is filled, we can reset the state
	requestStateReset(originalState);
	AudioDataReadyR = true;
}

void GMBBufferObj::setXFade(float _value)
{
	WaitForSingleObject(ghMutex,INFINITE);

	rlParams->xfade = _value;
	ReleaseMutex(ghMutex);
}

void GMBBufferObj::setVolL(float _value)
{
	WaitForSingleObject(ghMutex,INFINITE);

	rlParams->volL = _value;
	ReleaseMutex(ghMutex);
}

void GMBBufferObj::setVolR(float _value)
{
	WaitForSingleObject(ghMutex,INFINITE);

	rlParams->volR = _value;
	ReleaseMutex(ghMutex);
}

void GMBBufferObj::setFltCU(float _value)
{
	WaitForSingleObject(ghMutex,INFINITE);

	rlParams->fltr_cu = _value;
	ReleaseMutex(ghMutex);
}

void GMBBufferObj::setFltQ(float _value)
{
	WaitForSingleObject(ghMutex,INFINITE);

	rlParams->fltr_q = _value;
	ReleaseMutex(ghMutex);
}

float GMBBufferObj::getVolumeL()
{
	return getRLParams().volL;	
}

float GMBBufferObj::getVolumeR()
{
	return getRLParams().volR;
}

float GMBBufferObj::getFltCU()
{
	return getRLParams().fltr_cu;
}

float GMBBufferObj::getFltQ()
{
	return getRLParams().fltr_q;
}


float GMBBufferObj::getXFade()
{
	return getRLParams().xfade;
}


//void GMBBufferObj::posChangedL_setCallback(GMBSimpleObj instance)
//{
//	__OnPosLChangedFnPtr = inFn;
//}

ULONG GMBBufferObj::getPosL()
{
	ULONG pos;
	WaitForSingleObject(ghMutex, INFINITE);
	pos = Markers.Pos;
	ReleaseMutex(ghMutex);
	return pos;
}

void GMBBufferObj::posChangedR_setCallback(void(*inFn)(UINT))
{
	//__OnPosRChangedFnPtr = inFn;
	this->onPosChanged = inFn;
}

void GMBBufferObj::posChangedR(UINT newPos)
{
	if (onPosChanged)
	{
		onPosChanged(newPos);
	}
}

void GMBBufferObj::fire_posLChanged(ULONG x)
{
	/*ofstream olog(LOGPATH_EVENTS);
	
	if (__OnPosLChangedFnPtr != NULL)
	{
		olog << "Callback address is: " << addressof(__OnPosLChangedFnPtr) << endl;
		olog.close();
		*(__OnPosLChangedFnPtr(x));
	}*/
	
}

/**********************************Class GMBCircularBuffer*****************************************************
***************************************************************************************************************/
using namespace dmath;												//For GMBMod from discrete math 

GMBCircularBuffer::GMBCircularBuffer(int MAX_SIZE = 512)			//Default constructor
{
	pos = 0;
	ARR_LENGTH = MAX_SIZE;
	arr = new float[ARR_LENGTH];
}
	

void GMBCircularBuffer::write(float* input, int nElementsToWrite)
{
	int nPass1;
	int nPass2;

	//If nElements is more than the max length of the circular buffer, then only copy the last MAX elements 
	//into the circular buffer, since those are the only elements that will end up there even if 
	//we fill the whole thing the normal way.
	if (nElementsToWrite > ARR_LENGTH)
	{
		input = input + nElementsToWrite - ARR_LENGTH;
		nElementsToWrite = GMBMod(nElementsToWrite, ARR_LENGTH * sizeof(float));
	}

	//If we are about to write past the bounds of the cirular buffer, only write the first part. Otherwise,
	//write the whole thing with memcpy.
	if (pos + nElementsToWrite > ARR_LENGTH)
	{
		nPass1 = nElementsToWrite - pos;
		nPass2 = nElementsToWrite - nPass1;
		memcpy(arr + pos, input, nPass1 * sizeof(float));
		pos = 0;						//Now we are back at position 0 of the circular buffer.
		memcpy(arr, input + nElementsToWrite - nPass1 - nPass2, nPass2 * sizeof(float));
	}
	else
	{
		memcpy(arr + pos, input, nElementsToWrite * sizeof(float));
	}
}


void GMBCircularBuffer::write_reverse(GMBTypeArrayDescription<float>* _in)
{
	int c = 0;							//Another counter
	for (int i = 0; c < _in->nElements; ++i)
	{
		this[i] = *(_in->pArray - c);
		--c;
	}
}

void GMBCircularBuffer::write_mux(GMBTypeArrayDescription<float>* inputL, float gainL, GMBTypeArrayDescription<float>* inputR, float gainR)
{
	float temp = 0;

	if (inputL != NULL)
	{
		for (int i=0; i < inputL->nElements; ++i)
		{
			this[i] = (*(inputL->pArray + dmath::GMBMod(i, inputL->nElements))  * gainL );
		}
	}
	else 
	{
		for (int i=0; i < inputL->nElements; ++i)
		{
			this[i] = 0;
		} 
	}
	for (int i=0; i < inputL->nElements; ++i)
	{
		this[i] = (*(inputL->pArray + dmath::GMBMod(i, inputL->nElements))  * gainL );
	}

	for (int i=0; i < inputR->nElements; ++i)
	{
		temp = this->operator[](i);
		temp + *(inputR->pArray + GMBMod(i, inputR->nElements)) * gainR;
		this->operator[](i) = temp;
	}
}

//void GMBCircularBuffer::write_mux_reverse(GMBTypeArrayDescription<float>* inputL, float gainL, GMBTypeArrayDescription<float>* inputR, float gainR)
//{
//	int c = 0;							//Another counter
//	for (int i = 0; c < nElementsToWrite; ++i)
//	{
//		this[i] = *(inputL - c);
//		--c;
//	}
//}

float& GMBCircularBuffer::operator[](int i)
{
	int actualPos = GMBMod(i, ARR_LENGTH);
	return *(arr + GMBMod(i, ARR_LENGTH));
}



void GMBCircularBuffer::read(float* dst, int nElementsToRead)
{
	int nPass1;
	int nPass2;
	int loopDuration = nElementsToRead / ARR_LENGTH;
	int remainder;
	int j = 0;

	if (loopDuration >= 1)
	{
		remainder = GMBMod(nElementsToRead, ARR_LENGTH);
		
		while (j < loopDuration)
		{
			memcpy(dst + (j * ARR_LENGTH), arr + r_pos, (ARR_LENGTH - r_pos) * sizeof(float));
			if (ARR_LENGTH - r_pos != ARR_LENGTH)
			{
				memcpy(dst + (j * ARR_LENGTH) + (ARR_LENGTH - r_pos), arr, (r_pos) * sizeof(float) );
			}
			++j;
		}

		memcpy(dst + (j * ARR_LENGTH), arr + r_pos, remainder * sizeof(float));
		return;		//No need to go on to next section if we ended up here.
	}

	

	if (r_pos + nElementsToRead > ARR_LENGTH)
	{
		nPass1 = nElementsToRead - r_pos;
		nPass2 = nElementsToRead - nPass1;
		memcpy(dst, arr + r_pos, nPass1 * sizeof(float));	//Read to the upper bounds of the array.
		memcpy(dst + nPass1, arr, nPass2 * sizeof(float));	//Read the remainder of the array (located at the beginning)
	}
	else
	{
		memcpy(dst, arr + r_pos, nElementsToRead * sizeof(float));
	}
	return;
}

void GMBProcessArray_GainCoeffL(float* in, float coeff, int count)
{
	if (coeff > 1)
		coeff = 1;
	if (coeff < 0)
		coeff = 0;

	for (int i=0; i < count; ++i)
	{
		*(in + i) = *(in + i) * coeff;
	}
}

void GMBInitFilterConstants()
{
	xn_1L = 0;
	xn_2L = 0;
	xn_1R = 0;
	xn_2R = 0;

	yn_1L = 0;
	yn_2L = 0;
	yn_1R = 0;
	yn_2R = 0;
}

void GMBProcessArray_BiQuad2ndOrderLPF(float* src, float fc, float Q, int N)
{
	ofstream olog(LOGPATH_DSPFILTER);
	olog << "GMBProcessArray_BiQuad2ndOrderLPF says: Hello!\n";
	if (N <= 0)
		return;
	if (Q > 10)
		Q = 10;
	if (Q <= 0)
		Q = 0.707; 
	if (fc <= 0)
		fc = 500;
	if (fc > 20000)
		fc = 20000;

	olog << "Q is " << Q << endl;
	olog << "N is " << N << endl;
	olog << "fc is " << fc << endl;
	//Initialize our coefficients.
	d = 1 / Q;
	fc_r = (2 * PI_F * fc) / 44100.0;
	beta = 0.5 * (1 - ( (d / 2.0) * sin(fc_r) ) ) / ( 1 + (d / 2.0) * sin(fc_r) );
	gamma = (0.5 + beta) * cos(fc_r);
	a0 = (0.5 + beta - gamma);
	a1 = (0.5 + beta - gamma) / 2.0;
	a2 = a0;
	b1 = -2 * gamma;
	b2 = 2 * beta;

	olog << "fc_r is " << fc_r << endl;
	int n = 0;				//Counter
	

	while (n < N)
	{
		while (n < 5)
		{
			switch (n)
			{
				case 0:
					xnL = *(src + n);																//Store current sample as x(n)
					ynL = (a0 * xnL) + (a1 * xn_1L) + (a2 * xn_2L) - (b1 * yn_1L) - (b2 * yn_2L);	//Calculate the value of the output sample
					*(src + n) = ynL;																//Send the output sample to the output.
					break;	
				case 1:																				//Repeat case 0 for the right channel
					xnR = *(src + n);
					ynR = (a0 * xnR) + (a1 * xn_1R) + (a2 * xn_2R) - (b1 * yn_1R) - (b2 * yn_2R);
					*(src + n) = ynR;
					break;
				case 2:																				//Repeat case 0 for left channel but also take care of propagating prior samples through the one sample delay pipeline
					xn_1L = xnL;
					xnL = *(src + n);
					yn_1L = ynL;
					ynL = (a0 * xnL) + (a1 * xn_1L) + (a2 * xn_2L) - (b1 * yn_1L) - (b2 * yn_2L);
					/*xn_2L = xn_1L;
					xn_1L = *(src + n);*/
					*(src + n) = ynL;
					break;
				case 3:																				//Repeat case 2 for the right channel, etc etc...
					xn_1R = xnR;
					xnR = *(src + n);
					ynR = (a0 * xnR) + (a1 * xn_1R) + (a2 * xn_2R) - (b1 * yn_1R) - (b2 * yn_2R);
					//xn_1R = *(src + n);
					*(src + n) = ynR;
					break;
				case 4:
					xn_2L = xn_1L;
					xn_1L = xnL;
					xnL = *(src + n);
					yn_2L = yn_1L;
					yn_1L = ynL;
					ynL = (a0 * xnL) + (a1 * xn_1L) + (a2 * xn_2L) - (b1 * yn_1L) - (b2 * yn_2L);
					*(src + n) = ynL;
					break;
				case 5:
					yn_2R = yn_1R;
					yn_1R = ynR;
					xn_2R = xn_1R;
					xn_1R = xnR;
					xnR = *(src + n);
					ynR = (a0 * xnR) + (a1 * xn_1R) + (a2 * xn_2R) - (b1 * yn_1R) - (b2 * yn_2R);
					/*xn_2R = xn_1R;
					xn_1R = *(src + n);*/
					*(src + n) = ynR;
					break;
			}
			++n;
		}
		if (n % 2 == 0)
			{
				yn_2L = yn_1L;
				yn_1L = ynL;
				xn_2L = xn_1L;
				xn_1L = xnL;
				xnL = *(src + n);
				ynL = (a0 * xnL) + (a1 * xn_1L) + (a2 * xn_2L) - (b1 * yn_1L) - (b2 * yn_2L);
				*(src + n) = ynL;
			}
			else
			{
				yn_2R = yn_1R;
				yn_1R = ynR;
				xn_2R = xn_1R;
				xn_1R = xnR;
				xnR = *(src + n);
				ynR = (a0 * xnR) + (a1 * xn_1R) + (a2 * xn_2R) - (b1 * yn_1R) - (b2 * yn_2R);
				*(src + n) = ynR;
			}
			++n;
	}
	olog << "GMBProcessArray_BiQuad2ndOrderLPF says: Just finished one pass!\n";
	olog << "\tn ended up being " << n << endl;
	olog.close();
}


void DeinterleaveFloatArray(float* l, float* r, float* src, int src_length)
{
	int x = 0;			//Counter
	int y = 0;			//Counter

	for (int i=0; i < src_length; ++i)
	{
		if (i % 2 == 0)
		{
			*(l + x) = *(src + i);
			++x;
		}
		else
		{
			*(r + y) = *(src + i);
			++y;
		}
	}
}

void InterleaveFloatArray(float* l, float* r, float* dst, int dst_length)
{
	int x = 0;			//Counter
	int y = 0;			//Counter

	for (int i=0; i < dst_length; ++i)
	{
		if (i % 2 == 0)
		{
			*(dst + i) = *(l + x);
			++x;
		}
		else
		{
			*(dst + i) = *(r + y);
			++y;
		}
	}
}

