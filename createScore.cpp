#include "MIDItoScore.hpp"
#include <iomanip>
#include <sstream>
#include <algorithm>


bool isNumber(char ch) {
	return '0' <= ch && ch < '9';
}

bool isAlphabet(char ch) {
	return ('a' <= ch && ch <= 'g') || ('A' <= ch && ch <= 'G');
}

bool getNumber(int &num, size_t digit) {
	bool ret = true;
	num = 0;

	for (int i = 0; i < digit; i++) {
		char ch;

		std::cin >> ch;
		if (isNumber(ch)) {
			num *= 10;
			num += atoi(&ch);
		} else {
			ret = false;
			break;
		}
	}

	std::cin.ignore();

	return ret;
}

int searchTrack(const std::vector<midireader::Track> &tracks, const std::string searchName) {
	int trackNum = -1;

	for (const auto t : tracks) {
		if (t.name == searchName) {
			trackNum = t.trackNum;
			break;
		}
	}

	return trackNum;
}

bool isInclude(int val, int flag) {
	return (val & flag) == flag;
}


bool getInterval(std::string &interval) {
	bool ret = true;
	size_t progress = 0;

	while (true) {
		char ch;

		std::cin >> ch;

		if (ch == ' ')
			continue;

		switch (progress) {
		case 0:
			if (!isAlphabet(ch)) {
				ret = false;
				continue;
			}
			break;
		case 1:
			if (ch != '#' && !isNumber(ch)) {
				ret = false;
				continue;
			}
			break;
		case 2:
			if (!isNumber(ch)) {
				ret = false;
				continue;
			}
		}


		interval += ch;
		progress++;

		if (isNumber(ch))
			break;
	}

	std::cin.ignore();

	return ret;
}




int main() {

	using namespace midireader;
	using std::cin;
	using std::cout;

	bool loopFlag;

	// get class number
	int classNum;
	loopFlag = true;

	while (loopFlag) {
		cout << "組番号を入力して下さい(3桁)\n>";

		if (getNumber(classNum, 3)) {
			loopFlag = false;
		} else {
			cout << "[!] 入力エラーです．3桁の半角数字で入力して下さい\n";
		}
	}


	// get the midi file path
	MIDIReader midir;
	loopFlag = true;
	Status ret;

	while (loopFlag) {
		cout << "MIDIファイルへのパスを入力して下さい(\"や\'がついたままでもOKです)\n>";

		std::string filePath;
		char str[256];
		cin.getline(str, 256, '\n');
		filePath = str;


		// erase ' or "
		if (!filePath.empty()) {
			if (filePath.front() == '\'' || filePath.front() == '\"')
				filePath.erase(filePath.begin());
			if (filePath.back() == '\'' || filePath.back() == '\"')
				filePath.erase(filePath.end() - 1);
		}

		midir.setAdjustmentAmplitude(2);
		ret = midir.openAndRead(filePath);

		if (ret == Status::E_CANNOT_OPEN_FILE) {
			cout << "[!] ファイルが開けません.パスを確認して下さい\n";
		} else if (ret == Status::E_INVALID_ARG) {
			cout << "[!] パスが空です．\n";
		} else {
			loopFlag = false;
		}
	}


	cout << "\nMIDIファイルを読み込んでいます... ";

	// print result of reading the midi file
	switch (ret) {
	case midireader::Status::E_UNSUPPORTED_FORMAT:
		cout << "[!] このフォーマットはサポートされていません.\n";
		return 0;
	case midireader::Status::E_INVALID_FILE:
		cout << "[!] MIDIファイルが破損しています\n";
		return 0;
	case midireader::Status::S_OK:
		cout << "読み込み完了\n\n";
		break;
	case midireader::Status::S_NO_EMBED_TIMESIGNATURE:
		cout << "[!] MIDIファイルに拍子情報が埋め込まれていません.\n";
		return 0;
	default:
		break;
	}

	if (midir.getTempoEvent().empty()) {
		cout << "[!] MIDIファイルにテンポ情報が埋め込まれていません.\n";
		return 0;
	}




	// get interval
	cout << "打ち込みに使った音程を入力してください (例:C3, D#3)\n";


	std::vector<std::string> intervalList;
	for (size_t i = 0; i < 4; i++) {
		loopFlag = true;
		while (loopFlag) {

			// print lane position;
			// ex. if i = 1, print "□■□□"
			for (size_t j = 0; j < i; j++) cout << "□";
			cout << "■";
			for (size_t j = 4 - i - 1; j > 0; j--) cout << "□";

			cout << " 左から" << i+1 << "番目のレーンの音程 >";

			std::string str;
			if (getInterval(str)) {
				intervalList.push_back(str);
				loopFlag = false;
			} else {
				cout << "[!] 音名として正しくありません．\n";
			}
		}
	}


	// to upper
	for (auto &i : intervalList) {
		std::transform(i.begin(), i.end(), i.begin(), ::toupper);
	}


	// set note format
	miditoscore::MIDItoScore toscore;
	miditoscore::NoteFormat format;
	format.holdMaxVelocity = 63;
	format.laneAllocation = intervalList;


	// create score file name
	std::stringstream scoreName;
	time_t t = time(nullptr);
	tm *lt = localtime(&t);

	{
		using namespace std;
		scoreName << classNum;
		scoreName << '_';
		scoreName << setfill('0') << setw(2) << lt->tm_mon + 1;
		scoreName << setfill('0') << setw(2) << lt->tm_mday;
		scoreName << ".txt";
	}




	// ---------------------------------------------------
	// write score

	cout << "\n譜面データを作成します\n";

	std::ofstream score;
	score.open(scoreName.str());


	score << "begin:header\n\n";
	score << u8"id:<曲ID>" << '\n';
	score << u8"title:<曲名>" << '\n';
	score << u8"artist:<アーティスト名>" << '\n';


	// write tempo
	cout << "テンポ情報\n";
	score << '\n';

	const auto tempo = midir.getTempoEvent();
	for (const auto t : tempo) {
		using namespace std;

		// ex. tempo:001:1/0:120.000
		score << "tempo:"
			<< setfill('0') << setw(3) << t.bar
			<< ':'
			<< t.posInBar.get_str()
			<< ':'
			<< setw(6) << fixed << setprecision(3) << t.tempo
			<< '\n';

		cout << "小節:"
			<< setfill('0') << setw(3) << t.bar
			<< " 小節内位置:"
			<< t.posInBar.get_str()
			<< " テンポ:"
			<< setw(6) << fixed << setprecision(3) << t.tempo
			<< '\n';
	}


	// write time signature
	cout << "\n拍子情報\n";

	const auto beat = midir.getBeatEvent();
	for (const auto b : beat) {
		using namespace std;

		// ex. beat:001:4/4
		score << "beat:"
			<< setfill('0') << setw(3) << b.bar
			<< ':'
			<< b.beat.get_str()
			<< '\n';

		cout << "小節:"
			<< setfill('0') << setw(3) << b.bar
			<< " 拍子:"
			<< b.beat.get_str()
			<< '\n';
	}

	score << "\nend\n\n";


	// write note position
	for (char targetTrackName = '1'; targetTrackName <= '3'; targetTrackName++) {

		int trackNum = -1;
		const auto tracks = midir.getTrackList();
		trackNum = searchTrack(tracks, std::string(1, targetTrackName));
		if (trackNum < 0) {
			continue;
		}


		cout << '\n';
		switch (targetTrackName) {
		case '1':
			cout << "easy譜面を作成中です... ";
			score << "begin:easy\n\n";
			break;
		case '2':
			cout << "normal譜面を作成中です... ";
			score << "begin:normal\n\n";
			break;
		case '3':
			cout << "hard譜面を作成中です... ";
			score << "begin:hard\n\n";
			break;
		}


		auto ret = toscore.writeScore(score, format, midir, trackNum);

		score << "\nend\n\n";

		// print return value
		if (ret == miditoscore::Status::S_OK)
			cout << "完了\n";
		else {
			cout << "エラー\n";

			if (isInclude(ret, miditoscore::Status::E_EXIST_CONCURRENTNOTES)) {
				cout << "[!] 同じタイミングのノーツが存在しています．\n";
				cout << " ->長押しの終点と始点が重なっていないかチェックして下さい\n";
				cout << "-- 問題のあるノーツ --\n";

				int cnt = 0;
				const auto notes = toscore.getConcurrentNotes();
				for (auto n : notes) {
					using namespace std;
					cout << "小節:"
						<< setfill('0') << setw(3) << n.bar
						<< " 小節内位置:"
						<< n.posInBar.get_str()
						<< " 音程:"
						<< n.interval
						<< '\n';

					if (++cnt >= 10)
						break;
				}

				if (toscore.getConcurrentNotes().size() > 10)
					cout << "...他" << notes.size() - 10 << "コ\n";

				cout << '\n';
			}
			if (isInclude(ret, miditoscore::Status::S_EXIST_DEVIATEDNOTES)) {
				cout << "[!] 指定された音程に当てはまらないノーツが存在しています.\n";
				cout << "-- 問題のあるノーツ --\n";

				int cnt = 0;
				auto notes = toscore.getDeviatedNotes();
				for (auto n : notes) {
					using namespace std;
					cout << "小節:"
						<< setfill('0') << setw(3) << n.bar
						<< " 小節内位置:"
						<< n.posInBar.get_str()
						<< " 音程:"
						<< n.interval
						<< '\n';

					if (++cnt >= 10)
						break;
				}

				if (notes.size() > 10)
					cout << "...他" << notes.size() - 10 << "コ\n";

				cout << '\n';
			}
		}

		cout << "--ノーツ内訳-----\n";
		cout <<	"    |";
		for (auto i : intervalList) {
			cout << std::setfill(' ') << std::setw(4) << i << '|';
		}
		cout << '\n';
		cout << "hit |";
		for (auto i : intervalList) {
			cout << std::setfill(' ') << std::setw(4) << toscore.numofHitNotes(i) << '|';
		}
		cout << '\n';
		cout << "hold|";
		for (auto i : intervalList) {
			cout << std::setfill(' ') << std::setw(4) << toscore.numofHoldNotes(i) << '|';
		}
		cout << '\n';

		size_t cnt = 0;
		cout << "all |";
		for (auto i : intervalList) {
			cout << std::setfill(' ') << std::setw(4)
				<< toscore.numofHoldNotes(i) + toscore.numofHitNotes(i) << '|';

			cnt += toscore.numofHoldNotes(i) + toscore.numofHitNotes(i);
		}
		cout << '\n';
		cout << "total:" << cnt << '\n';



	}


	score.close();
	midir.close();

	cout << "\nプログラムを終了するにはEnterを押してください\n";
	cin.ignore();

	return 0;
}