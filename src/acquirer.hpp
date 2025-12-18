#pragma once

#include "hw_peripherals.hpp"

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

#define MEM_SIZE 1024 * 1024 * 1024 // 1 GB

class Server;

class Acquirer
{
public:
	Acquirer(std::string, float);
	~Acquirer();

	void StartThreads(Server *);
	void StopThreads();
	void Start();
	std::vector<std::string> Stop();
	void StartRecording();
	void StartRecording(std::string);
	void PauseRecording();
	void ResumeRecording();
	std::vector<std::string> StopRecording();
	std::vector<std::string> SaveRecording();
	void TagRecording(std::string);

	int SetT2(float);
	int SetVG(double, int);
	int SetVsetpoint(double, int);

	bool running_;
	std::atomic_bool acquiring_;
	std::atomic_bool recording_;
	std::atomic_bool paused_;

private:
	void AcquireData();
	void ProcessData(Server *);

	std::mutex dataMutex;
	std::condition_variable acqCV;
	std::condition_variable dataCV;

	std::jthread acqThread_;
	std::jthread procThread_;
	float T2_;
	long int iter_;
	unsigned char use_buffer_;
	unsigned char proc_buffer_;
	char pingpong_A_[BUF_LEN];
	char pingpong_B_[BUF_LEN];
	void *memblock_;
	long memoffset_;
	std::string filename_;
	std::string data_folder_;
	std::string tags_;
};