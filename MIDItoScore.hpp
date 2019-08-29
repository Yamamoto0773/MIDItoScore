/*
 (c) 2018/8/16 written by Nanami Yamamoto
 
 This class read MIDI file, and then create a file written the score data of music game.


 ################################
 # about score format

 --- score data format -----
 s:nnn:kk...
 ---------------------------
 s		:lane number
 nnn	:bar number
 kk...	:express the note position and the note type
 

 --- example score data ----
 0:020:0000100000000000
 ---------------------------
 mean:
 lane			:0
 note position	:20th bar and offset 16th-note x 4,
 note type		:1
 */





#ifndef _MIDITOSCORE_HPP_
#define _MIDITOSCORE_HPP_


#include "MIDIReader.hpp"

namespace miditoscore {


	struct NoteFormat {
		math::Fraction holdMinLength;
		std::vector<std::string> laneAllocation;
	};

	enum class NoteType {
		NONE = 0,
		HIT,
		HOLD_BEGIN,
		HOLD_END
	};

	struct ScoreNote {
		NoteType type;
		const midireader::NoteEvent* evt;

		ScoreNote(NoteType _type, const midireader::NoteEvent* _evt) :
			type(_type), evt(_evt) {};
	};

	namespace Status {
		constexpr int S_OK = 0b0000;
		constexpr int E_EXIST_CONCURRENTNOTES = 0b0001;
		constexpr int E_CANNOT_OPEN_FILE = 0b0010;
		constexpr int S_EXIST_DEVIATEDNOTES = 0b0100;
	};


	bool Success(int s);
	bool Failed(int s);



	class MIDItoScore {
		using noteevent_const_itr_t = std::vector<midireader::NoteEvent>::const_iterator;

	public:
		MIDItoScore();
		~MIDItoScore();

		int writeScore(const std::string &fileName, const NoteFormat &format, const std::vector<midireader::NoteEvent> &notes, size_t trackNum);
		int writeScore(std::ostream &stream, const NoteFormat &format, const std::vector<midireader::NoteEvent> &notes, size_t trackNum);

		int createScoreString(const std::vector<ScoreNote>& scoreNotes, std::string& scoreString);

		const std::vector<midireader::NoteEvent> &getConcurrentNotes() const { return concurrentNotes; }
		const std::vector<midireader::NoteEvent> &getDeviatedNotes() const { return deviatedNotes; }
		size_t numofHoldNotes(const std::string &interval) const;
		size_t numofHitNotes(const std::string &interval) const;

	private:

		struct NoteAggregate {
			NoteAggregate() : hit(0), hold(0) {}
			~NoteAggregate() {}

			void reset() { hit = 0, hold = 0; }
			void increment(NoteType type) {
				switch (type) {
				case miditoscore::NoteType::HIT:
					hit++;
					break;
				case miditoscore::NoteType::HOLD_END:
					hold++;
					break;
				default:
					break;
				}
			}

			size_t hit;
			size_t hold;
		};


		NoteFormat noteFormat;
		std::vector<midireader::NoteEvent> concurrentNotes;
		std::vector<midireader::NoteEvent> deviatedNotes;
		std::vector<NoteAggregate> noteAggregate;

		int selectNoteLane(const NoteFormat &format, const midireader::NoteEvent &note);

		void clear();
		

	};

}

#endif // !_MIDITOSCORE_HPP_
