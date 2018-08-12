#include "MIDIReader.hpp"

#include <array>

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

	MIDIReader::~MIDIReader() {}

	MIDIReader::MIDIReader(const std::string &fileName) {
		open(fileName);
	}

	Status MIDIReader::open(const std::string &fileName) {
		if (fileName.empty())
			return Status::E_INVALID_ARG;

		if (midi.is_open())
			midi.close();


		midi.open(fileName, std::ios_base::binary);
		if (!midi)
			return Status::E_CANNOT_OPEN_FILE;

		return Status::S_OK;
	}


	Status MIDIReader::load() {
		if (!midi.is_open())
			return Status::E_SET_NOFILE;


		loadHeader();


		for (int i = 0; i < header.numofTrack; i++) {
			loadTrack();

		}





		return Status::S_OK;
	}

	const MIDIHeader & MIDIReader::getHeader() {
		return header;
	}

	const std::vector<NoteEvent>& MIDIReader::getNoteEvent() {
		return noteEvent;
	}


	void MIDIReader::close() {
		midi.close();
		noteEvent.clear();

	}

	int MIDIReader::read(std::string & str, size_t byte) {
		char ch;

		eraseAll(str);

		size_t i;
		for (i = 0; i < byte; i++) {
			midi.read(&ch, 1);
			str += ch;
		}


		return i;
	}

	int MIDIReader::readVariableLenNumber(long & num) {
		num = 0;
		int byteCnt = 0;

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

	void MIDIReader::eraseAll(std::string & str) {
		str.erase(str.cbegin(), str.cend());
	}

	Status MIDIReader::loadHeader() {
		std::string tmp;

		// get chunk name
		std::string chunk;
		read(chunk, 4);

		// get chunk length
		read(tmp, 4);
		int chunkLength = btoi(tmp);


		if (chunk == "MThd") {

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
			header.resolutionUnit = resolutionUnit >> 1;


		} else {
			return Status::E_INVALID_FILE;
		}

		return Status::S_OK;
	}

	Status MIDIReader::loadTrack() {

		// get chunk name
		std::string chunk;
		read(chunk, 4);

		// get chunk data length
		std::string tmp;
		read(tmp, 4);
		long chunkLength = btoi(tmp);


		if (chunk != "MTrk")
			return Status::E_INVALID_FILE;


		long totalTime = 0;
		while (1) {

			// get delta time
			long deltaTime;
			readVariableLenNumber(deltaTime);

			totalTime += deltaTime;

			// get status byte
			read(tmp, 1);
			unsigned char status = btoi(tmp);


			if ((status >> 4) == MidiEvent::NoteOn) {

				NoteEvent evt;

				evt.channel = status & 0x0f;

				// get note number
				read(tmp, 1);
				evt.noteNum = btoi(tmp);
				// get velocity
				read(tmp, 1);
				evt.velocity = btoi(tmp);

				evt.totalTime = totalTime;
				evt.type = MidiEvent::NoteOn;

				noteEvent.push_back(evt);

			} else if ((status >> 4) == MidiEvent::NoteOff) {

				NoteEvent evt;

				evt.channel = status & 0x0f;

				// get note number
				read(tmp, 1);
				evt.noteNum = btoi(tmp);
				// get velocity
				read(tmp, 1);
				evt.velocity = btoi(tmp);

				evt.totalTime = totalTime;
				evt.type = MidiEvent::NoteOff;

				noteEvent.push_back(evt);

			} else if ((status == MidiEvent::MetaEvent)) {

				// get event type
				read(tmp, 1);
				unsigned char eventType = btoi(tmp);
				// get data length
				long dataLength;
				readVariableLenNumber(dataLength);


				if (eventType == MetaEvent::InstName) {

					std::string instName;
					read(instName, dataLength);

				} else if (eventType == MetaEvent::TrackEnd) {

					// exit from loop
					break;

				} else if (eventType == MetaEvent::Tempo) {

					read(tmp, dataLength);
					float tempo = 60.0f*1e6f/btoi(tmp);

				} else if (eventType == MetaEvent::TimeSignature) {

					read(tmp, 1);
					int numer = btoi(tmp);
					read(tmp, 1);
					int denom = static_cast<int>(pow(2, btoi(tmp)));

					// nothing to do
					read(tmp, 2);

				} // metaEvent

			} // midiEvent

		} // while (1)


		return Status::S_OK;
	}



};


