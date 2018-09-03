#include "MIDItoScore.hpp"

#include <iostream>
#include <iomanip>
#include <algorithm>

namespace miditoscore {


	MIDItoScore::MIDItoScore() {}

	MIDItoScore::~MIDItoScore() {}

	midireader::Status MIDItoScore::writeScore(const std::string & fileName, const NoteFormat & format, const midireader::MIDIReader &midi, size_t trackNum) {
		std::ofstream scoreFile(fileName.c_str());
		if (!scoreFile.is_open())
			return midireader::Status::E_CANNOT_OPEN_FILE;

		return writeScore(scoreFile, format, midi, trackNum);
	}

	midireader::Status MIDItoScore::writeScore(std::ostream & stream, const NoteFormat & format, const midireader::MIDIReader &midi, size_t trackNum) {
		using namespace midireader;

		const auto &noteEvent = midi.getNoteEvent(trackNum);


		size_t currentBar = 1;
		noteevent_const_itr_t begin_it = noteEvent.cbegin();
		noteevent_const_itr_t end_it = begin_it;

		std::vector<std::string> scoreData;
		scoreData.resize(format.laneAllocation.size());
		std::vector<bool> isHoldStarted;
		isHoldStarted.resize(format.laneAllocation.size());

		while (end_it != noteEvent.cend()) {

			// calculate range of the note event in current bar
			for (end_it = begin_it; end_it != noteEvent.cend(); end_it++) {
				if (end_it->bar != currentBar) break;
			};

			createScoreInBuffer(scoreData, isHoldStarted, format, begin_it, end_it);

			// write the score data to file.
			for (auto sd = scoreData.cbegin(); sd != scoreData.cend(); sd++) {
				if (sd->size() > 0) {
					//printf("%d:%03d:%s\n", sd - scoreData.cbegin(), currentBar, sd->c_str());

					using namespace std;
					stream << sd - scoreData.cbegin() << ':'
						<< setfill('0') << setw(3) << currentBar << ':'
						<< sd->c_str() << '\n';
				}
			}


			// go to next bar
			begin_it = end_it;
			currentBar++;
		}


		return Status::S_OK;
	}

	int MIDItoScore::selectNoteLane(const NoteFormat &format, const noteevent_const_itr_t &note) {
		auto lane_it = std::find(format.laneAllocation.begin(), format.laneAllocation.end(), note->interval);
		if (lane_it == format.laneAllocation.cend())
			return -1;

		return static_cast<int>(lane_it - format.laneAllocation.cbegin());
	}

	void MIDItoScore::createDataLengthList(std::vector<size_t>& dataLength, const NoteFormat & format, std::vector<bool> &isHoldStarted, const noteevent_const_itr_t &begin, const noteevent_const_itr_t &end) {
		size_t numofLane = format.laneAllocation.size();

		dataLength.clear();
		dataLength.resize(numofLane);


		std::vector<math::Fraction> barUnit;
		barUnit.resize(numofLane);
		for (auto &b : barUnit) b.set(0);

		std::vector<bool> exist;
		exist.resize(numofLane);

		for (auto it = begin; it != end; it++) {
			math::Fraction pos(it->posInBar);

			int laneNumber = selectNoteLane(format, it);
			if (laneNumber >= 0 &&
				NoteType::NONE != selectNoteType(it, format, isHoldStarted.begin() + laneNumber)) {
				exist.at(laneNumber) = true;
				math::adjustDenom(barUnit.at(laneNumber), pos);
			}
		}

		for (size_t i = 0; i < dataLength.size(); i++) {
			if (exist.at(i))
				dataLength.at(i) = barUnit.at(i).get().d;
			else
				dataLength.at(i) = 0;
		}
	}


	void MIDItoScore::createScoreInBuffer(std::vector<std::string>& scoreData, std::vector<bool> &isHoldStarted, const NoteFormat & format, const noteevent_const_itr_t &begin_it, const noteevent_const_itr_t &end_it) {

		for (auto &sd : scoreData) sd.clear();

		// calculate minimum unit which can express position of the note in a bar
		std::vector<size_t> dataLengthList;
		{
			auto copy = isHoldStarted;
			createDataLengthList(dataLengthList, format, copy, begin_it, end_it);
		}

		// create empty score data
		auto len = dataLengthList.cbegin();
		auto b = scoreData.begin();
		for (; len != dataLengthList.cend(); len++, b++) {
			for (size_t i = 0; i < *len; i++) (*b) += '0';
		}


		// add note to the score data
		for (auto it = begin_it; it != end_it; it++) {

			// get lane number
			int laneNum = selectNoteLane(format, it);
			if (laneNum < 0)
				continue;


			// select note type
			NoteType noteType = selectNoteType(it, format, isHoldStarted.begin() + laneNum);
			if (noteType == NoteType::NONE)
				continue;


			// calculate note offset
			math::Fraction pos(it->posInBar);
			math::Fraction unit(1, static_cast<int>(dataLengthList.at(laneNum)));
			math::adjustDenom(pos, unit);
			int offset = pos.get().n;


			scoreData.at(laneNum).at(offset) = ('0' + static_cast<int>(noteType));
		}
	}

	NoteType MIDItoScore::selectNoteType(const noteevent_const_itr_t &event, const NoteFormat &format, std::vector<bool>::iterator isHoldStarted) {
		NoteType noteType = NoteType::NONE;

		if (event->velocity <= format.holdMaxVelocity) {

			if (event->type == midireader::MidiEvent::NoteOn) {
				noteType = NoteType::HOLD_BEGIN;
				*isHoldStarted = true;
			} else if (*isHoldStarted) {
				noteType = NoteType::HOLD_END;
				*isHoldStarted = false;
			}

		} else {

			if (event->type == midireader::MidiEvent::NoteOn) {
				noteType = NoteType::HIT;
			}

		}

		return noteType;
	}



}