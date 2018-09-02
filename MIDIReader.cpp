#include "MIDIReader.hpp"

#include <cmath>


namespace midireader {


	// convert byte (stored in std::string) to int
	int btoi(const std::string &str) {
		int ans = 0;

		for (auto ch : str) {
			unsigned higherBit = (ch & 0xf0) >> 4;
			unsigned lowerBit = (ch & 0x0f);

			ans <<= 8;
			ans |= ((higherBit << 4) | lowerBit);
		}

		return ans;
	}

	bool operator==(unsigned char L, MetaEvent R) {
		return (L == static_cast<unsigned char>(R));
	}

	bool operator!=(unsigned char L, MetaEvent R) {
		return !(L == R);
	}

	bool operator==(unsigned char L, MidiEvent R) {
		return (L == static_cast<unsigned char>(R));
	}

	bool operator!=(unsigned char L, MidiEvent R) {
		return !(L == R);
	}

	bool Success(Status s) { return static_cast<int>(s) >= 0; };
	bool Failed(Status s) { return static_cast<int>(s) < 0; };


	MIDIReader::MIDIReader() {}

	MIDIReader::~MIDIReader() {
		close();
	}

	MIDIReader::MIDIReader(const std::string &fileName) {
		openAndRead(fileName);
	}

	Status MIDIReader::openAndRead(const std::string &fileName) {
		if (fileName.empty())
			return Status::E_INVALID_ARG;

		if (midi.is_open())
			close();


		midi.open(fileName, std::ios_base::binary);
		if (!midi)
			return Status::E_CANNOT_OPEN_FILE;

		return readAll();
	}

	const MIDIHeader & MIDIReader::getHeader() const {
		return header;
	}

	const std::vector<NoteEvent>& MIDIReader::getNoteEvent(size_t trackNum) const {
		if (trackNum-1 >= noteEvent.size())
			return dummyEvent;

		return noteEvent.at(trackNum-1);
	}

	const std::vector<std::vector<NoteEvent>>& MIDIReader::getNoteEvent() const {
		return noteEvent;
	}

	const std::vector<BeatEvent>& MIDIReader::getBeatEvent() const {
		return beatEvent;
	}

	const std::vector<TempoEvent>& MIDIReader::getTempoEvent() const {
		return tempoEvent;
	}

	const std::vector<Track>& MIDIReader::getTrackList() const {
		return trackList;
	}

	const std::string & MIDIReader::getTitle() const {
		return musicTitle;
	}

	void MIDIReader::setAdjustmentAmplitude(size_t midiTime) {
		adjustAmplitude = midiTime;
	}

	void MIDIReader::close() {
		midi.close();
		header = { 0, 0, 0 };
		musicTitle.clear();
		noteEvent.clear();
		beatEvent.clear();
		tempoEvent.clear();
		trackList.clear();
	}





	Status MIDIReader::readAll() {
		Status ret;

		// read midi file
		if (Failed(ret = readHeader()))
			return ret;


		// ready for std::vector of note event
		noteEvent.resize(header.numofTrack);


		// read tracks
		for (int i = 1; i < header.numofTrack + 1; i++) {
			if (Failed(ret = readTrack(i)))
				return ret;
		}



		// calculate bar and posInBar, and add them to event.
		for (auto &tracknote : noteEvent) {
			for (auto &note : tracknote) {

				// find the most suitable fraction which express position of the note.
				ScoreTime scoreTime = calcBestScoreTime(note.time, 256);

				note.bar = scoreTime.bar;
				note.posInBar = scoreTime.posInBar;

			}
		}

		for (auto &e : beatEvent) {
			auto ret = calcScoreTime(e.time);
			e.bar = ret.bar;
		}

		for (auto &e : tempoEvent) {
			auto ret = calcScoreTime(e.time);
			e.bar = ret.bar;
		}



		if (beatEvent.empty())
			return Status::S_NO_EMBED_TIMESIGNATURE;


		return Status::S_OK;
	}

	size_t MIDIReader::read(std::string & str, size_t byte) {
		char ch;

		eraseAll(str);

		size_t i;
		for (i = 0; i < byte; i++) {
			midi.read(&ch, 1);
			str += ch;
		}


		return i;
	}

	size_t MIDIReader::readVariableLenNumber(long & num) {
		num = 0;
		size_t byteCnt = 0;

		while (true) {
			char ch;

			midi.read(&ch, 1);
			byteCnt++;

			num <<= 7;
			num |= (ch & 0b0111'1111);

			if (!(ch >> 7)) {
				// if 7th bit is 0, exit from loop
				break;
			}
		}

		return byteCnt;
	}

	MIDIReader::ScoreTime MIDIReader::calcScoreTime(long midiTime) {
		MIDIReader::ScoreTime ans(0, math::Fraction(0));

		if (beatEvent.empty())
			return ans;


		// ---------------------------
		// count the bar
		ans.bar = 1;
		auto resolution = static_cast<int>(4 * header.resolutionUnit * getBeat(0));
		long time = 0;

		while (midiTime - time >= resolution) {
			time += resolution;
			ans.bar++;

			// get resolutin unit for next bar
			resolution = static_cast<int>(4 * header.resolutionUnit * getBeat(time));
		}


		// ---------------------------
		// calculation position in bar
		ans.posInBar = { static_cast<int>(midiTime - time), resolution };
		ans.posInBar.reduce();
		if (ans.posInBar == 0) {
			ans.posInBar.set(0);
		}


		return ans;
	}

	MIDIReader::ScoreTime MIDIReader::calcBestScoreTime(long &midiTime, size_t threshold) {
		int amplitude_i = static_cast<int>(adjustAmplitude);
		long origin = midiTime;

		ScoreTime bestAns = calcScoreTime(origin);

		if (bestAns.posInBar.get().d > threshold) {

			for (int offset = -amplitude_i; offset <= +amplitude_i; offset++) {
				// skip if midiTime + offset => <underflow>
				if (offset < 0 && origin < std::abs(offset)) continue;
				if (offset == 0) continue;

				auto scoreTime = calcScoreTime(origin + offset);
				if (scoreTime.posInBar.get().d < bestAns.posInBar.get().d) {
					bestAns = scoreTime;
					midiTime = origin + offset;
				}
			}

		}


		return bestAns;
	}

	const math::Fraction & MIDIReader::getBeat(long miditime) {
		for (auto rit = beatEvent.crbegin(); rit != beatEvent.crend(); rit++) {
			if (rit->time <= miditime)
				return rit->beat;
		}

		return beatEvent.cbegin()->beat;
	}

	std::string MIDIReader::toIntervalStr(int noteNum) {
		std::string intervalStr;


		// add alphabet
		switch (noteNum%12) {
		case 0:
		case 1:
			intervalStr.push_back('C');
			break;
		case 2:
		case 3:
			intervalStr.push_back('D');
			break;
		case 4:
			intervalStr.push_back('E');
			break;
		case 5:
		case 6:
			intervalStr.push_back('F');
			break;
		case 7:
		case 8:
			intervalStr.push_back('G');
			break;
		case 9:
		case 10:
			intervalStr.push_back('A');
			break;
		case 11:
			intervalStr.push_back('B');
			break;
		}

		// add sharp
		switch (noteNum%12) {
		case 1:
		case 3:
		case 6:
		case 8:
		case 10:
			intervalStr.push_back('#');
			break;
		}

		// add number
		intervalStr += std::to_string(noteNum/12 -1);


		return intervalStr;
	}

	void MIDIReader::eraseAll(std::string & str) {
		str.erase(str.cbegin(), str.cend());
	}

	Status MIDIReader::readHeader() {
		std::string tmp;


		// move file pointer
		midi.seekg(0, std::ios_base::beg);

		// get chunk name
		std::string chunk;
		read(chunk, 4);

		// get chunk length
		read(tmp, 4);
		int chunkLength = btoi(tmp);


		if (chunk != "MThd")
			return Status::E_INVALID_FILE;


		// check midi format
		read(tmp, 2);
		int format = btoi(tmp);
		if (format == 2) {
			// [SMF error] SMF FORMAT 2 is unsupported.
			return Status::E_UNSUPPORTED_FORMAT;
		}


		// get the number of tracks
		read(tmp, 2);
		int numofTrack = btoi(tmp);


		// check the resolution unit
		read(tmp, 2);
		int resolutionUnit = btoi(tmp);
		if (resolutionUnit >> 15) {
			// [SMF error] this TIME UNIT FORMAT is unsupported.
			return Status::E_UNSUPPORTED_FORMAT;
		}



		header.format = format;
		header.numofTrack = numofTrack;
		header.resolutionUnit = resolutionUnit;



		return Status::S_OK;
	}

	Status MIDIReader::readTrack(int trackNum) {

		if (trackNum < 1)
			return Status::E_INVALID_ARG;

		std::string tmp;


		// move file pointer
		midi.seekg(0, std::ios_base::beg);

		for (int i = 0; i < trackNum; i++) {
			read(tmp, 4); // chunk name

			if ((i == 0 && tmp != "MThd") ||
				(i > 0  && tmp != "MTrk"))
				return Status::E_INVALID_FILE;

			eraseAll(tmp);
			read(tmp, 4); // data length
			long chunklength = btoi(tmp);

			midi.seekg(chunklength, std::ios_base::cur);
		}


		auto event_it = noteEvent.begin() + trackNum - 1;



		// get chunk name
		std::string chunk;
		read(chunk, 4);

		// get chunk data length
		read(tmp, 4);
		long chunkLength = btoi(tmp);


		if (chunk != "MTrk")
			return Status::E_INVALID_FILE;



		// add track
		trackList.push_back(Track(trackNum, ""));



		long totalTime = 0;
		while (1) {

			// get delta time
			long deltaTime;
			readVariableLenNumber(deltaTime);

			totalTime += deltaTime;

			// get status byte
			read(tmp, 1);
			unsigned char status = btoi(tmp);
			unsigned char status_upper = status >> 4;

			
			// Note On/Off
			if (status_upper == 0x9 || status_upper == 0x8) {

				NoteEvent evt;

				evt.channel = status & 0x0f;
				// get note number
				read(tmp, 1);
				evt.interval = toIntervalStr(btoi(tmp));
				// get velocity
				read(tmp, 1);
				evt.velocity = btoi(tmp);

				evt.time = totalTime;

				if (status_upper == 0x9)
					evt.type = MidiEvent::NoteOn;
				else
					evt.type = MidiEvent::NoteOff;

				event_it->push_back(evt);
			
			// Meta Event
			} else if (status == 0xff) {

				// get event type
				read(tmp, 1);
				unsigned char eventType = btoi(tmp);
				// get data length
				long dataLength;
				readVariableLenNumber(dataLength);


				if (eventType == MetaEvent::InstName) {

					std::string instName;
					read(instName, dataLength);

					// search a element which has same track number 
					size_t subscript = findTrack(trackNum) - trackList.cbegin();

					if (header.format == 0) {
						musicTitle = instName;
						trackList.at(subscript).name = instName;
					} else {
						if (trackNum == 1)
							musicTitle = instName;
						else
							trackList.at(subscript).name = instName;
					}

				} else if (eventType == MetaEvent::TrackEnd) {

					// exit from loop
					break;

				} else if (eventType == MetaEvent::Tempo) {

					read(tmp, dataLength);
					float tempo = 60.0f*1e6f/btoi(tmp);

					tempoEvent.push_back(
						TempoEvent(totalTime, 0, tempo)
					);

				} else if (eventType == MetaEvent::TimeSignature) {

					read(tmp, 1);
					int numer = btoi(tmp);
					read(tmp, 1);
					int denom = static_cast<int>(std::pow(2, btoi(tmp)));

					// nothing to do
					read(tmp, 2);

					beatEvent.push_back(
						BeatEvent(totalTime, 0, math::Fraction(numer, denom))
					);

				} else {

					// nothing to do
					read(tmp, dataLength);

				} // metaEvent



			// nothing to do in following events
			} else if (status_upper == 0xa) {
				// polyphonic key pressure
				read(tmp, 2);
			} else if (status_upper == 0xb) {
				// controll change
				read(tmp, 2);
				read(tmp, 1);
				if (btoi(tmp) > 0x7f)
					midi.seekg(-1, std::ios::cur);
			} else if (status_upper == 0xc) {
				// program change
				read(tmp, 1);
			} else if (status_upper == 0xd) {
				// channel pressure
				read(tmp, 1);
			} else if (status_upper == 0xe) {
				// pitch bend
				read(tmp, 2);
			} else if (status == 0xf0 || status == 0xf7) {
				// SysEx event
				long dataLength;
				readVariableLenNumber(dataLength);

			} // midiEvent

		} // while (1)


		return Status::S_OK;
	}

	std::vector<Track>::const_iterator MIDIReader::findTrack(size_t trackNum) {

		for (auto it = trackList.cbegin(); it != trackList.cend(); it++) {
			if (it->trackNum == trackNum) {
				return it;
			}
		}

		return trackList.cend();
	}



};


