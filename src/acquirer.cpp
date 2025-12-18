#include "acquirer.hpp"
#include "server.hpp"

#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <chrono>
#include <iomanip>
#include <cstring>

#define IDS_LATEX(x, y) "$I_{ds_{" + (x) + "}}=-" + (y) + "\\text{ }\\mu \\text{A}$"
#define VG_LATEX(x, y) "$V_{g_{" + (x) + "}}=-" + (y) + "\\text{ V}$"

Acquirer::Acquirer(std::string data_folder, float T2) : acquiring_(false), recording_(false), paused_(false),
												   T2_(T2), iter_(0), use_buffer_(0), proc_buffer_(0), memoffset_(0),
												   data_folder_(data_folder), tags_("")
{
	// Check if the data folder exists and create it if it doesn't
	struct stat info;

	if (stat(data_folder_.c_str(), &info) != 0)
	{
		std::cout << "Creating data folder..." << '\n';
		if (mkdir(data_folder_.c_str(), 0777) != 0)
		{
			std::cerr << "Error creating data folder" << '\n';
			return;
		}
	}

	// Allocate memory for the data
	memblock_ = malloc(MEM_SIZE);
	if (memblock_ == NULL)
	{
		std::cerr << "Error allocating memory" << '\n';
		return;
	}

	std::cout << "Memory allocated: " << std::hex << memblock_ << std::dec << '\n';
	// SetT2(T2);
}

Acquirer::~Acquirer()
{
	// Free the memory
	free(memblock_);
	std::cout << "Memory freed." << '\n';
}

// SendData callback
void Acquirer::StartThreads(Server *server)
{
	running_ = true;
	std::cout << "Starting acquisition and processing threads..." << '\n';
	acqThread_ = jthread(&Acquirer::AcquireData, this);
	procThread_ = jthread(&Acquirer::ProcessData, this, server);
	std::cout << "Threads started." << '\n';
}

void Acquirer::StopThreads()
{
	std::unique_lock<std::mutex> lock(dataMutex);
	acquiring_ = true;
	acqCV.notify_all();
	running_ = false;
	dataCV.notify_all();
	lock.unlock();
}

void Acquirer::Start()
{
	acquiring_ = true;
	Set_T2lock(0);
	acqCV.notify_one();
}

void Acquirer::StartRecording()
{
	recording_ = true;
	paused_ = false;

	if (!acquiring_)
		Start();
}

void Acquirer::StartRecording(string filename)
{
	filename_ = filename;
	tags_ = "time,tag\n";

	StartRecording();
}

void Acquirer::PauseRecording()
{
	paused_ = true;
}

void Acquirer::ResumeRecording()
{
	paused_ = false;
}

vector<string> Acquirer::StopRecording()
{
	recording_ = false;
	paused_ = false;

	return SaveRecording();
}

vector<string> Acquirer::SaveRecording()
{
	auto now = std::system_clock::now();
	time_t now_c = std::chrono::system_clock::to_time_t(now);

	std::stringstream ss;
	ss << std::put_time(std::localtime(&now_c), "%Y%m%d_%H%M%S");
	std::string filename = data_folder_ + filename_ + "_" + ss.str() + ".bin";
	std::string tags_filename = data_folder_ + filename_ + "_" + ss.str() + ".tags";

	// Open the file
	FILE *fp = fopen(filename.c_str(), "wb");
	if (fp == NULL)
	{
		std::cerr << "Error opening file" << '\n';
		return std::vector<std::string>{};
	}

	// Write the data
	fwrite(memblock_, 1, memoffset_, fp);

	// Close the file
	fclose(fp);

	// Open the tags file
	fp = fopen(tags_filename.c_str(), "w");
	if (fp == NULL)
	{
		std::cerr << "Error opening tags file" << '\n';
		return std::vector<std::string>{};
	}

	// Write the tags
	fwrite(tags_.c_str(), 1, tags_.length(), fp);

	// Close the file
	fclose(fp);

	// Reset the memory offset
	memoffset_ = 0;

	return std::vector<std::string>{filename, tags_filename};
}

std::vector<std::string> Acquirer::Stop()
{
	Set_T2lock(1);
	acquiring_ = false;
	iter_ = 0;

	if (recording_)
		return StopRecording();
	else
		return vector<string>{};
}

void Acquirer::TagRecording(std::string tag)
{
	tags_ += std::to_string((float)(memoffset_) / 4 * T2_ / 1e6) + "," + tag + "\n";
}

void Acquirer::AcquireData()
{
	use_buffer_ = BUFFER_A;
	proc_buffer_ = BUFFER_B;

	uint8_t ack0, ack0_prec;
	char dummytxbuf[BUF_LEN];

	while (running_)
	{
		ack0_prec = bcm2835_gpio_lev(ACK0);
		if (iter_ == 0)
			ack0_prec = (ack0_prec == 0) ? 1 : 0;

		// assert REQ low, thus requesting data to the SPI port
		bcm2835_gpio_write(REQ, LOW);
		// deassert REQ
		bcm2835_gpio_write(REQ, HIGH);

		// wait for the dsPic to acknowledge the SPI transfer
		do
		{
			ack0 = bcm2835_gpio_lev(ACK0);
		} while (ack0 == ack0_prec);

		// bcm2835_gpio_write(TP3, HIGH);

		// cout << "ACQ: Pre-LOCK " << iter_ << endl;
		// Wait for the acquisition to start
		std::unique_lock<std::mutex> lock(dataMutex);
		// Use a condition variable to start the acquisition
		acqCV.wait(lock, [this]() -> bool
				   { return acquiring_ && running_; });
		// cout << "ACQ: LOCK " << iter_ << endl;

		// Read data via SPI
		if (use_buffer_ == BUFFER_A)
		{
			bcm2835_spi_transfernb(&dummytxbuf[0], &pingpong_A_[0], BUF_LEN);
		}
		else
		{
			bcm2835_spi_transfernb(&dummytxbuf[0], &pingpong_B_[0], BUF_LEN);
		}

		// Unlock the mutex
		lock.unlock();
		// cout << "ACQ: UNLOCK " << iter_ << endl;

		proc_buffer_ = (use_buffer_ == BUFFER_A) ? BUFFER_A : BUFFER_B;
		use_buffer_ = (use_buffer_ == BUFFER_A) ? BUFFER_B : BUFFER_A;

		// Notify the processing thread that the data is ready
		dataCV.notify_one();

		iter_++;
	}
}

void Acquirer::ProcessData(Server *server)
{
	while (running_)
	{
		// Wait for the data to be ready
		// cout << "PROC: Pre-LOCK " << iter_ << endl;
		std::unique_lock<std::mutex> lock(dataMutex);
		// cout << "PROC: LOCK " << iter_ << endl;
		dataCV.wait(lock);

		char *data = (proc_buffer_ == BUFFER_A) ? pingpong_A_ : pingpong_B_;
		if (recording_ && !paused_)
		{
			std::memcpy((char *)memblock_ + memoffset_, data, BUF_LEN);
		}

		// Send the data to the server
		server->SendData(data);

		// Unlock the mutex
		lock.unlock();
		// cout << "PROC: UNLOCK " << iter_ << endl;

		if (recording_ && !paused_)
		{
			memoffset_ = (memoffset_ >= MEM_SIZE - BUF_LEN) ? 0 : memoffset_ + BUF_LEN;
			// cout << "memoffset: " << memoffset_ << endl;
		}
	}
}

int Acquirer::SetT2(float value)
{
	T2_ = value;
	return Set_T2(T2_);
}

int Acquirer::SetVG(double value, int channel)
{
	// set value precision to 2 decimal digits (V)
	std::stringstream ss;
	ss << std::fixed << std::setprecision(2) << value;
	TagRecording(VG_LATEX(std::to_string(channel), ss.str()));

	return Set_VG(value, channel);
}

int Acquirer::SetVsetpoint(double value, int channel)
{
	// set value precision to 2 decimal digits (uA)
	std::stringstream ss;
	ss << std::fixed << std::setprecision(2) << value;
	TagRecording(IDS_LATEX(std::to_string(channel), ss.str()));

	return Set_Vsetpoint(value, channel);
}
