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
		int holdMaxVelocity;
		std::vector<std::string> laneAllocation;
	};


	enum class NoteType {
		NONE = 0,
		HIT,
		HOLD_BEGIN,
		HOLD_END
	};


	class MIDItoScore {
	public:
		MIDItoScore();
		~MIDItoScore();


		midireader::Status readMidi(const std::string &fileName);

		midireader::Status createScore(const std::string &fileName, const NoteFormat &format);
		midireader::Status createScore(std::ostream &stream, const NoteFormat &format);



	private:
		midireader::MIDIReader midi;

		midireader::Status prevStatus;


		using noteevent_const_itr_t = std::vector<midireader::NoteEvent>::const_iterator;

		int selectNoteLane(const NoteFormat &format, const noteevent_const_itr_t &note);
		NoteType selectNoteType(const noteevent_const_itr_t &note, const NoteFormat &format, std::vector<bool>::iterator isHoldStarted);
		
		void createDataLengthList(
			std::vector<size_t> &dataLength, 
			const NoteFormat &format,
			std::vector<bool> &isHoldStarted,
			const noteevent_const_itr_t &begin, 
			const noteevent_const_itr_t &end
			);

		void createScoreInBuffer(
			std::vector<std::string> &buffer,
			std::vector<bool> &isHoldStarted,
			const NoteFormat &format,
			const noteevent_const_itr_t &begin_it,
			const noteevent_const_itr_t &end_it
			);

		
			
		

	};

}

#endif // !_MIDITOSCORE_HPP_
