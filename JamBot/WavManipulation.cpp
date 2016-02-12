// WavSnippet.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "soundfile.h"
#include "WavManipulation.h"
#include "InputChannelReader.h"
#include "Helpers.h"
#include "Constants.h"
#include "fftw3.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <ctime>
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <cmath>
#include <queue>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//#ifndef OLDCPP
//#include <iostream>
//using namespace std;
//#else
//#include <iostream.h>
//#endif
using namespace std;

vector<float> WavManipulation::realTimeBuffer = vector<float>();
vector<SecAnlys> WavManipulation::freqList = vector<SecAnlys>();
static queue<float> frequency = queue<float>();
static queue<string> section = queue<string>();
static bool checkfrequency;


SongSection::SongSection(string n, double t){
	Name = n;
	startTime = t;
}
SongSection::~SongSection(){
}
SecAnlys::SecAnlys(){
}
SecAnlys::SecAnlys(string n, vector<float> t, int d){
	name = n;
	pitch = t;
	duration = d;
}
SecAnlys::~SecAnlys(){
}
WavManipulation::WavManipulation(){
	durations = vector<short>();
	filenames = vector<string>();
	frequency = queue<float>();
	section = queue<string>();
	checkfrequency = true;
	terminate = false;
	durationCounter = 0;
	directoryPath = "";
}
WavManipulation::~WavManipulation(){
	durations.clear();
	filenames.clear();
	freqList.clear();
	realTimeBuffer.clear();
	durationCounter = 0;
	directoryPath = "";
}


Helpers::SongElement getElement(string name){
	string temp = name.substr(0, name.length() - 4);
	Helpers::SongElement retElement = Helpers::NIL;
	if (temp == "Intro" || temp == "intro" || temp == "INTRO"){
		retElement = Helpers::INTRO;
	}
	if (temp == "Verse" || temp == "verse" || temp == "VERSE"){
		retElement = Helpers::VERSE;
	}
	if (temp == "Chorus" || temp == "chorus" || temp == "CHORUS"){
		retElement = Helpers::CHORUS;
	}
	if (temp == "Bridge" || temp == "bridge" || temp == "BRIDGE"){
		retElement = Helpers::BRIDGE;
	}
	if (temp == "Outro" || temp == "outro" || temp == "OUTRO"){
		retElement = Helpers::OUTRO;
	}
	return retElement;
}

int getNum(string name){
	string temp = name.substr(name.length() - 5, 1);
	int num = 0;
	try{
		num = std::stoi(temp.c_str());
	}
	catch (invalid_argument e){
		num = 0;
	}
	return num;
}

void WavManipulation::inputData(vector<float> buffer){
	realTimeBuffer = buffer;
}

///////////////////////////////////////////
///	wavComparison: Compares a wav file with realtime input
///					Runs until it finds a match or hits the end of the files
/// Input: short realTimeBuffer[] - the buffered values of the real time input
///		   string - directoryPath - the directory path name
/// Output: char*
Helpers::SongStructure WavManipulation::wavComparison() {

	unsigned short i, j, channel;
	int error_counter = 1000;
	double threshold = 0.0009;
	Helpers::SongElement element = Helpers::NIL;
	int num = 0;
	float sample = 0;

	for (j = 0; j < filenames.size(); j++){
		short k = 0;
		string readFile = directoryPath + filenames[j];
		SoundFileRead  insound1(readFile.c_str());
		durationCounter = durations[j];
		for (i = 0; i < insound1.getSamples(); i++) {
			if (error_counter == 0){
				break;
			}
			for (channel = 0; channel < insound1.getChannels(); channel++) {
				sample = (float)insound1.getCurrentSampleDouble(channel);
				sample = sample - realTimeBuffer[k + (channel)];
				if (abs(sample) > threshold){
					error_counter--;
				}
				else{
					error_counter++;
				}
			}
			k += 2;
			insound1.incrementSample();
		}
		if (error_counter > 0){
			element = getElement(filenames[i]);
			num = getNum(filenames[i]);
			return Helpers::SongStructure(element, num);
		}
	}
	return Helpers::SongStructure(element, 0);
}
///////////////////////////////////////////
///	snipAudio: Snips the audio file into sections, all wav files using the names in names vector
/// Input: vector: names - The names of each sections in order of the time they appear in the song
///		   vector: startTimes - The times (in seconds) of each section
///		   vector: durationTimes - The length (in seconds) of each section
///		   filepath - The path of the file being cut
/// Output: VOID
void WavManipulation::snipAudio(vector<string> names, vector<short> startTimes, vector<short> durationTimes, string filePath, string filename){

	SoundFileRead insound((filePath + filename).c_str());
	SoundHeader header = insound;
	int startSample = 0;
	int stopSample = 0;
	int n = 0;
	int numSnips = 0;

	for (unsigned int i = 0; i < startTimes.size(); i++){
		durations.push_back((short)ceil(durationTimes[i] / 0.2));  //store durations as a number of 200ms bursts
		string outName = names[i] + ".wav";
		filenames.push_back(names[i]); //store file names for the sections
		SoundFileWrite outsound(outName.c_str(), header);
		startSample = (short)(startTimes[i] * insound.getSrate() + 0.5);  //starting sample
		stopSample = (short)((startTimes[i] + 0.2) * insound.getSrate() + 0.5);//ending sample
		n = stopSample - startSample; //number of samples
		insound.gotoSample(startSample);
		for (int j = 0; j < n; j++){
			for (int k = 0; k < header.getChannels(); k++) { //for each channel of each sample
				//write the sample from original to current file
				outsound.writeSampleDouble(insound.getCurrentSampleDouble(k));
			}
			insound.incrementSample();
		}

	}
}
void WavManipulation::comparisonPolling(){
	//Queue pull
	vector<float> indata;
	std::deque<Helpers::SongStructure>  structurequeue;
	//
	while (true){
		if (realTimeBuffer.empty()){
			continue;
		}
		if (durationCounter > 0){
			durationCounter--;
		}
		else{
			Helpers::SongStructure section = wavComparison();
			structurequeue.push_back(section); //This needs to be adjusted to push to mohammed's queue
		}
	}
}
void WavManipulation::startSnip(){
	string filename = "crunchy_bass_swag.flv.wav";
	string filepath = "../";
	vector<string> names = { "Intro", "Chorus1", "Verse1", "Outro" };
	vector<short> durations = { 1, 1, 1, 1 };
	vector<short> startTimes = { 1, 3, 5, 7 };
	snipAudio(names, startTimes, durations, filepath, filename);
}

void clearqueue(){
	queue<float> empty;
	swap(frequency, empty);
}
bool WavManipulation::pushFrequency(float in){
	if (frequency.size() < AUDIO_BUF_SIZE)
		frequency.push(in);
	return frequency.size() < AUDIO_BUF_SIZE;
}
bool WavManipulation::readFrequency(){
	return checkfrequency;
}
void WavManipulation::startanalysis(){
	vector<SongSection> secs = vector<SongSection>();
	secs.push_back(SongSection("Verse1", 23.0));
	secs.push_back(SongSection("Chorus", 54.0));
	secs.push_back(SongSection("verse2", 89.0));
	secs.push_back(SongSection("Chorus", 122.0));
	dataStore("song1", secs);
	freqSnip("C:\\Users\\emerson\\Downloads\\", "Boston_More_than_a_FeelingVocals_Only.wav", "song1");
}

void WavManipulation::dataStore(string filename, vector<SongSection> sections){
	ofstream song;
	ostringstream s;
	song.open(filename + ".csv");
	song << "Section Name, Start Time\n";
	string str = "";
	for (int i = 0; i < sections.size(); i++){
		song << sections[i].Name + "," + to_string(sections[i].startTime) + "\n";
	}

}
bool WavManipulation::checkrepeats(string name){
	for (int i = 0; i < freqList.size(); i++){
		if (freqList[i].name == name){
			return true;
		}
	}
	return false;
}
float WavManipulation::freqtonote(float in){
	return 12 * log2f(in / 440) + 49;
}
float WavManipulation::notetofreq(int in){
	return pow(2, ((in - 49) / 12)) * 440;
}
float WavManipulation::threshold(float in){
	int note = round((double)freqtonote(in));
	float lower = (in - notetofreq(note - 1)) / 2;
	float upper = (notetofreq(note + 1) - in) / 2;
	float thresh = (upper + lower);
	return thresh;
}

float WavManipulation::hannFunction(int n)
{
	double inner = (2 * M_PI * n) / (FFT_SIZE - 1);
	return (float)(0.5 * (1.0 - cos(inner)));
}
float WavManipulation::freqAnalysis(vector<float> data){

	float *in;
	fftwf_complex *out;
	fftwf_plan frequencyPlan;
	InputChannelReader inchannel;
	float magnitude;
	short fileSamples;
	float maxDensity = 0.0;
	float frequency;
	int maxIndex;
	in = (float*)fftwf_malloc(sizeof(float)* FFT_SIZE);
	out = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)* OUTPUT_SIZE);
	frequencyPlan = fftwf_plan_dft_r2c_1d(FFT_SIZE, in, out, FFTW_ESTIMATE);
	for (int i = 0; i < 1200; i++)
	{
		// Use every-other sample
		if (i < FFT_SIZE)
		{
			in[i] = data[i] * hannFunction(i);
		}

	}
	// Get frequency of wave
	fftwf_execute(frequencyPlan);

	for (int i = 0; i < 1200; i++)
	{
		magnitude = (float)sqrt(pow(out[i][0], 2) + pow(out[i][1], 2));

			if (magnitude > maxDensity)
			{
				maxDensity = magnitude;
				maxIndex = i;
			}
	}
	frequency = maxIndex * SAMPLE_RATE / OUTPUT_SIZE;
	return frequency;
}

void WavManipulation::freqcomparison(){
	float inFreq = 0.0;
	float freq = 0.0;
	float t = 0.0;
	int point = 0;
	int part = 0;
	int num = 0;
	int j = 0;
	int ticks = 0;
	int duration = 0;
	char * err_str;
	Helpers::SongElement element = Helpers::NIL;
	while (!terminate){

		while (ticks < duration){
			checkfrequency = false;
			clearqueue();
			ticks++;
		}
		checkfrequency = true;
		try{
			while (j < 3){
				while (frequency.empty()){ if (terminate){ return; } }
				inFreq = frequency.front();
				frequency.pop();
				if (point == 0){
					for (int i = 0; i < freqList.size(); i++){
						freq = freqList[i].pitch[j];
						t = threshold(freq);
						if (abs(freq - inFreq) < t){
							part = i;
							point++;
							break;
						}
					}
				}
				freq = freqList[part].pitch[j];
				t = threshold(freq);
				if (abs(freq - inFreq) < t){
					point++;
					j++;
				}
				else{
					j++;
				}
				ticks++;
			}
			if (point > 1){
				element = getElement(freqList[part].name);
				num = getNum(freqList[part].name);
				//push this to queue for Mohamed Helpers::SongStructure(element, num);
				duration = freqList[part].duration;
				Helpers::print_debug(freqList[part].name.c_str());
				ticks = 0;
			}
		}
		catch (exception e){
			err_str = "";
			strcat(err_str, "ERROR: WavManipulation: ");
			strcat(err_str, e.what());
			strcat(err_str, "\n");
			Helpers::print_debug(err_str);
		}

	}	
	if (terminate) Helpers::print_debug("WavManipulation: terminated.\n");
	else Helpers::print_debug("WavManipulation: stopped.\n");
}

void WavManipulation::freqSnip(string filePath, string filename, string csvname){
	ifstream file(csvname + ".csv");
	SoundFileRead insound((filePath + filename).c_str());
	SoundHeader header = insound;
	vector<string> names;
	vector<double> times;
	int startSample = 0;
	int stopSample = 0;
	int n = 0;
	int numSnips = 0;
	string value = "";
	bool even = true;
	SecAnlys section;
	int length = floor((double)insound.getSamples() / (double)insound.getSrate());
	getline(file, value, ',');
	getline(file, value, '\n');
	while (!file.eof()){

		if (even){
			getline(file, value, ',');
			names.push_back(value);
		}
		else{
			getline(file, value, '\n');
			times.push_back(stod(value));
		}
		cout << value << endl;
		even = !even;
	}
	double freq = 0;
	for (unsigned int i = 0; i < times.size(); i++){
		vector<float> list;
		for (int l = 0; l < 3; l++){
			vector<float> snippet;
			startSample = ((times[i] + (l * 0.2)) * insound.getSrate() + 0.5);  //starting sample
			stopSample = ((times[i] + (l * 0.2) + 0.2) * insound.getSrate() + 0.5);//ending sample
			n = stopSample - startSample; //number of samples
			insound.gotoSample(startSample);
			for (int j = 0; j < n; j++){
				//for (int k = 0; k < insound.getChannels(); k++){
				snippet.push_back((float)insound.getCurrentSampleDouble(0));
				insound.incrementSample();
				//}
			}
			freq = freqAnalysis(snippet);
			list.push_back(freq);
			snippet.clear();
		}

		if (i < times.size() - 1){
			section = SecAnlys(names[i], list, ceil((times[i + 1] - times[i]) / 0.2));
		}
		else {
			section = SecAnlys(names[i], list, ceil((length - times[i]) / 0.2));
		}
		if (!checkrepeats(names[i])){
			Helpers::print_debug(names[i].c_str());
			freqList.push_back(section);
		}
	}
	
}

void WavManipulation::start(){
	freqcomparison();
}
void WavManipulation::stop(){
	terminate = true;
}