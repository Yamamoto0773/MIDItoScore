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




		
		

		return Status::S_OK;
	}

	const MIDIHeader & MIDIReader::getHeader() {
		return header;
	}


	void MIDIReader::close() {
		midi.close();


	}

	int MIDIReader::read(std::string & str, size_t byte) {
		char ch;

		size_t i;
		for (i = 0; i < byte; i++) {
			midi.read(&ch, 1);
			str += ch;
		}


		return i;
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
		eraseAll(tmp);
		read(tmp, 4);
		int chunkLength = btoi(tmp);

		if (chunk == "MThd") {

			// check midi format
			eraseAll(tmp);
			read(tmp, 2);
			int format = btoi(tmp);
			if (format == 2) {
				// [SMF error] SMF FORMAT 2 is unsupported.
				return Status::E_UNSUPPORTED_FORMAT;
			}


			// get the number of tracks
			eraseAll(tmp);
			read(tmp, 2);
			int numofTrack = btoi(tmp);


			// check the resolution unit
			eraseAll(tmp);
			read(tmp, 2);
			int resolutionUnit = btoi(tmp);
			if (resolutionUnit >> 15) {
				// [SMF error] this TIME UNIT FORMAT is unsupported.
				return Status::E_UNSUPPORTED_FORMAT;
			}



			header.format			= format;
			header.numofTrack		= numofTrack;
			header.resolutionUnit	= resolutionUnit >> 1;


		} else {
			return Status::E_INVALID_FILE;
		}

		return Status::S_OK;
	}



	/*
	`可変長数値をファイルから取得
	戻り値は固定長数値
	*/
	int readVariableLengthNum(std::ifstream &ifs, int *byteCnt) {
		if (!ifs) return 0;
		if (byteCnt) *byteCnt = 0;

		int n = 0;
		while (true) {
			char tmp = 0;

			ifs.read(&tmp, 1);
			if (byteCnt) (*byteCnt)++;

			int tmp2 = (int)tmp;

			n <<= 7;
			n |= (tmp2&0b01111111);

			if (!(tmp2 >> 7)) {
				// ビット7が0なら取得終了
				break;
			}
		}

		return n;
	}


	

};


