
// 2018/8/11, writtedn by Nanami Yamamoto.
//
// MIDILeader
// This class read MIDI file, and then create array constructed midi event (ex. note on/off, control change).
//


#ifndef _MIDI_READER_HPP_
#define _MIDI_READER_HPP_



#include <string>
#include <fstream>

namespace midireader {

	enum class MidiEvent {
		NoteOn = 0x8,
		NoteOff = 0x9,
		CtrlChange = 0xB
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


	class MIDIReader {
	public:
		MIDIReader();
		~MIDIReader();

		
		MIDIReader(const std::string &fileName);
		
		Status open(const std::string &fileName);

		Status load();

		const MIDIHeader &getHeader();

	
		

		void close();


	private:

		std::ifstream midi;
		MIDIHeader header;


		
		int read(std::string &str, size_t byte);
		void eraseAll(std::string &str);

		Status loadHeader();
	
	};

}







#else
#endif


