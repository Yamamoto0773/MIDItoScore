
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

#include "Fraction.hpp"


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
		S_OK = 0,
		S_NO_EMBED_TIMESIGNATURE = 1,
	};

	bool Success(Status s);
	bool Failed(Status s);


	struct MIDIHeader {
		int format;
		int numofTrack;
		int resolutionUnit;
	};

	struct BeatEvent {
		BeatEvent(long time, int bar, math::Fraction beat) {
			this->time = time;
			this->bar = bar;
			this->beat = beat;
		};

		~BeatEvent() {};

		long time;
		int bar;
		math::Fraction beat;
	};


	struct TempoEvent {
		TempoEvent(long time, int bar, float tempo) {
			this->time = time;
			this->bar = bar;
			this->tempo = tempo;
		}

		long time;
		int bar;
		float tempo;
		math::Fraction posInBar;
	};

	struct NoteEvent {
		MidiEvent type;
		int channel;
		long time;
		int bar;
		math::Fraction posInBar;

		std::string interval;
		int velocity;
	};

	struct Track {
		Track(int trackNum, std::string name) {
			this->trackNum = trackNum;
			this->name = name;
		}
		int trackNum;
		std::string name;
	};





	class MIDIReader {
	public:
		MIDIReader();
		MIDIReader(const std::string &fileName);

		~MIDIReader();

		// isYamaha ... specify type of the MIDI note number.
		// if 440Hz expressed as "A3", pass true (based YAMAHA)
		// if 440Hz expressed as "A4", pass false (based International)
		Status openAndRead(const std::string &fileName, bool isYamaha=true);

		const MIDIHeader &getHeader() const;
		// notice: When you want to get the note event of 1st track, call as "getNoteEvent(1)"
		const std::vector<NoteEvent> &getNoteEvent(size_t trackNum) const;
		const std::vector<std::vector<NoteEvent>> &getNoteEvent() const;
		const std::vector<BeatEvent> &getBeatEvent() const;
		const std::vector<TempoEvent> &getTempoEvent() const;
		// notice: return value is std::vector<Track>
		const std::vector<Track> &getTrackList() const;
		const std::string &getTitle() const;

		// set amplitude in adjusting timing of the note event
		// notice : When you call this function, please call it before openAndRead()
		void setAdjustmentAmplitude(size_t midiTime = 0);

		void close();

	private:

		std::ifstream midi;

		MIDIHeader header;
		std::string musicTitle;
		std::vector<std::vector<NoteEvent>> noteEvent;
		std::vector<BeatEvent> beatEvent;
		std::vector<TempoEvent> tempoEvent;
		std::vector<Track> trackList;

		bool isYamaha;


		// for amplitude in adjusting timing of the note event.
		// default value : 0
		size_t adjustAmplitude;


		// for out of range access
		const std::vector<NoteEvent> dummyEvent;


		// read whole midi file
		Status readAll();

		size_t read(std::string &str, size_t byte);
		size_t readVariableLenNumber(long &num);

		struct ScoreTime {
			ScoreTime(int bar, math::Fraction posInBar) {
				this->bar = bar;
				this->posInBar = posInBar;
			}

			int bar;
			math::Fraction posInBar;
		};

		ScoreTime calcScoreTime(long midiTime);
		ScoreTime calcBestScoreTime(long &midiTime, size_t threshold);


		const math::Fraction &getBeat(long miditime);

		std::string toIntervalStr(int noteNum, bool isYamaha);

		void eraseAll(std::string &str);

		Status readHeader();

		// notice: when read the 1st track, call as "readTrack(1)"
		Status readTrack(int trackNum);

		std::vector<Track>::const_iterator findTrack(size_t trackNum);


	};

}


#else
#endif
