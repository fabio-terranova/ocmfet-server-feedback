#pragma once

#include "hw_peripherals.hpp"

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

using namespace std;

#define MEM_SIZE 1024 * 1024 * 1024 // 1 GB

class Server;

class Acquirer
{
public:
	Acquirer(string, float);
	~Acquirer();

	void StartThreads(Server *);
	void StopThreads();
	void Start();
	vector<string> Stop();
	void StartRecording();
	void StartRecording(string);
	void PauseRecording();
	void ResumeRecording();
	vector<string> StopRecording();
	vector<string> SaveRecording();
	void TagRecording(string);

	int SetT2(float);
	int SetVG(double, int);
	int SetVsetpoint(double, int);

	bool running_;
	atomic_bool acquiring_;
	atomic_bool recording_;
	atomic_bool paused_;

private:
	void AcquireData();
	void ProcessData(Server *);

	mutex dataMutex;
	condition_variable acqCV;
	condition_variable dataCV;

	jthread acqThread_;
	jthread procThread_;
	
	float T2_;
	long int iter_;
	unsigned char use_buffer_;
	unsigned char proc_buffer_;
	char pingpong_A_[BUF_LEN];
	char pingpong_B_[BUF_LEN];
	void *memblock_;
	long memoffset_;
	string filename_;
	string data_folder_;
	string tags_;
};