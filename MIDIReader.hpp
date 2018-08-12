
// 2018/8/11, written by Nanami Yamamoto.
//
// MIDIReader
// This class read MIDI file, and then create array constructed midi event (ex. note on/off, control change).
//


#ifndef _MIDI_READER_HPP_
#define _MIDI_READER_HPP_



#include <string>
#include <vector>
#include <fstream>

namespace midireader {

	enum class MidiEvent {
		NoteOff = 0x8,
		NoteOn = 0x9,
		MetaEvent = 0xff,
	};

	enum class MetaEvent {
		InstName = 0x03,
		TrackEnd = 0x2f,
		Tempo = 0x51,
		TimeSignature = 0x58
	};


	enum class Status {
		E_CANNOT_OPEN_FILE = -1,
		E_INVALID_ARG = -2,
		E_SET_NOFILE = -3,
		E_UNSUPPORTED_FORMAT = -4,
		E_INVALID_FILE = -5,
		S_Keep = 0,
		S_OK = 1,
	};


	struct MIDIHeader {
		int format;
		int numofTrack;
		int resolutionUnit;
	};

	struct NoteEvent {
		MidiEvent type;
		int channel;
		long totalTime;
		int noteNum;
		int velocity;
	};


	class MIDIReader {
	public:
		MIDIReader();
		MIDIReader(const std::string &fileName);

		~MIDIReader();
		
		
		Status open(const std::string &fileName);

		Status load();

		const MIDIHeader &getHeader();
		const std::vector<NoteEvent> &getNoteEvent();
	
		

		void close();


	private:

		std::ifstream midi;

		std::vector<NoteEvent> noteEvent;
		MIDIHeader header;
		


		
		int read(std::string &str, size_t byte);
		int readVariableLenNumber(long &num);

		void eraseAll(std::string &str);

		Status loadHeader();
		Status loadTrack();
	
	};

}







#else
#endif


