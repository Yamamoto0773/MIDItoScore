#include "MIDItoScore.hpp"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <array>


bool isNumber(char ch) {
	return '0' <= ch && ch <= '9';
}

bool isIntervalAlphabet(char ch) {
	return ('a' <= ch && ch <= 'g') || ('A' <= ch && ch <= 'G');
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

bool toNumber(const std::string& str, int *number = nullptr) {
	size_t numEndedPos;
	int n;
	try {
		n = std::stoi(str, &numEndedPos);
	} catch (const std::exception&) {
		return false;
	}

	if (numEndedPos != str.length())
		return false;

	if (number) {
		*number = n;
	}

	return true;
}

bool toIntervalStr(std::string &str, std::string &intervalStr) {
	intervalStr.clear();

	if (str.length() < 2)
		return false;

	if (!isIntervalAlphabet(str.at(0)))
		return false;

	constexpr std::array<char, 5> sharpAttachable = {'C', 'D', 'F', 'G', 'A'};
	const bool isAttachable = 
		std::find_if(
			sharpAttachable.cbegin(),
			sharpAttachable.cend(),
			[&](char ch) { return ch ==std::toupper(str.at(0)); }
		) != sharpAttachable.cend();

	if (!isAttachable && str.at(1) == '#')
		return false;

	std::string numStr;
	if (str.at(1) == '#') {
		numStr = str.substr(2);
		intervalStr += str.substr(0, 2);
	} else {
		numStr = str.substr(1);
		intervalStr += str.substr(0, 1);
	}

	int octave;
	if (!toNumber(numStr, &octave))
		return false;

	intervalStr += std::to_string(octave);

	return true;
}

bool toFraction(std::string& str, math::Fraction& frac) {
	using std::cin;

	size_t slashPos = str.find('/');
	if (slashPos == std::string::npos)
		return false;

	int numer, denom;
	try {
		const auto numerStr = str.substr(0, slashPos);
		numer = std::stoi(numerStr);
		const auto denomStr = str.substr(slashPos + 1);
		denom = std::stoi(denomStr);

		frac.set(numer, denom); // may occured that denom is zero exception
	} catch (const std::exception&) {
		return false;
	}
	
	if (frac <= 0)
		return false;

	return true;
}

int main() {
	using namespace midireader;
	using std::cin;
	using std::cout;

	bool loopFlag = true;

	// get class number
	cout << "組番号を入力して下さい(3桁)\n";
	std::string classNumStr;
	while (loopFlag) {
		cout << ">";
		std::getline(cin, classNumStr);

		if (classNumStr.length() == 3 && toNumber(classNumStr)) {
			break;
		} else {
			cout << "[!] 入力エラーです．3桁の半角数字で入力して下さい\n";
		}
	}

	// get the midi file path
	MIDIReader midir;
	loopFlag = true;
	Status ret;

	cout << "MIDIファイルへのパスを入力して下さい(\"や\'がついたままでもOKです)\n";
	while (loopFlag) {
		cout << ">";
		std::string filePath;
		std::getline(cin, filePath);

		// erase ' or "
		if (!filePath.empty()) {
			if (filePath.front() == '\'' || filePath.front() == '\"')
				filePath.erase(filePath.begin());
			if (filePath.back() == '\'' || filePath.back() == '\"')
				filePath.erase(filePath.end() - 1);
		}

		midir.setAdjustmentAmplitude(2, 1024);
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
	cout << "打ち込みに使った音程を入力してください\n"
		<< "(例: 音名で入力する場合:C3,D#3  番号で入力する場合:60, 63)\n";

	std::vector<std::string> intervalStrings(4);
	std::vector<int> intervalNumbers(4, -1);
	bool givedIntervalAsStr = false;
	for (int lane = 3; lane >= 0; lane--) {
		while (true) {
			// print lane position;
			// ex. if i = 1, print "□■□□"
			for (size_t i = 0; i < 4; i++) {
				if (i == lane)
					cout << "■";
				else
					cout << "□";
			}

			cout << " 右から" << 4 - lane << "番目のレーンの音程 >";

			std::string inputStr, intervalStr;
			std::getline(cin, inputStr);

			int n;
			if (toIntervalStr(inputStr, intervalStr)) {
				intervalStrings.at(lane) = intervalStr;
				givedIntervalAsStr = true;
				break;
			} else if (toNumber(inputStr, &n) && n >= 0) {
				intervalNumbers.at(lane) = n;
				break;
			} else {
				cout << "[!] 音程として正しくありません．\n";
			}
		}
	}

	cout << '\n';

	bool isYamaha = false;
	if (givedIntervalAsStr) {
		cout << "DAWの一番低い音程は C-2 ですか？ [y/n]\n"
			<< ">";
		std::string answer;
		std::getline(cin, answer);
		if (answer == "y") {
			isYamaha = true;
		}

		// convert interval string to number
		for (size_t i = 0; i < 4; i++) {
			if (intervalNumbers.at(i) == -1) {
				intervalNumbers.at(i) = midireader::toIntervalNum(intervalStrings.at(i), isYamaha);
			}
		}
	}

	// get hold minimal length
	cout << "\n";
	cout << "長押しノーツだと見なす，最小の長さを入力してください (例: 1/16)\n"
		 << "(16分音符1つ分以上の長さのノーツを 長押しにしたい場合は 1/16と入力)\n";

	math::Fraction holdMinLen;
	while (true) {
		cout << ">";
		
		std::string str;
		std::getline(cin, str, '\n');

		if (toFraction(str, holdMinLen)) {
			break;
		} else {
			cout << "[!] 0より大きい分数を入力してください\n";
		}
	}

	// set note format
	miditoscore::MIDItoScore toscore;
	miditoscore::NoteFormat format;
	format.holdMinLength = holdMinLen;
	format.laneAllocation = intervalNumbers;
	format.allowedMaxDivision = 1024;

	// create score file name
	std::stringstream scoreName;
	time_t t = time(nullptr);
	tm *lt = localtime(&t);

	{
		using namespace std;
		scoreName << classNumStr;
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
	score << u8"id:曲ID" << '\n';
	score << u8"title:曲名" << '\n';
	score << u8"artist:アーティスト名" << '\n';

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

		auto ret = toscore.writeScore(score, format, midir.getNoteEvent(trackNum), trackNum);

		score << "\nend\n\n";

		// print return value
		if (ret == miditoscore::Status::S_OK)
			cout << "完了\n";
		else {
			cout << "エラー\n";

			if (isInclude(ret, miditoscore::Status::E_EXIST_CONCURRENTNOTES)) {
				cout << "[!] 同じタイミングのノーツが存在しています．\n";
				cout << " ->長押しの後にあるノーツと繋がっていないかチェックしてください\n";
				cout << " ->ノーツが重なっていないかチェックしてください\n";
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
						<< (givedIntervalAsStr ? 
							midireader::toIntervalStr(n.interval, isYamaha) : std::to_string(n.interval))
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
						<< (givedIntervalAsStr ?
							midireader::toIntervalStr(n.interval, isYamaha) : std::to_string(n.interval))
						<< '\n';

					if (++cnt >= 10)
						break;
				}

				if (notes.size() > 10)
					cout << "...他" << notes.size() - 10 << "コ\n";

				cout << '\n';
			}
			if (isInclude(ret, miditoscore::Status::E_EXIST_NONDISCRETENOTES)) {
				cout << "[!] クォンタイズされていないノーツが存在しています.\n";
				cout << " -> 許容できる1小節の分割数は" << format.allowedMaxDivision << "です\n";
				cout << "-- 問題のあるノーツ --\n";

				int cnt = 0;
				auto notes = toscore.getNonDiscreteNotes();
				for (auto n : notes) {
					using namespace std;
					cout << "小節:"
						<< setfill('0') << setw(3) << n.bar
						<< " 小節内位置:"
						<< n.posInBar.get_str()
						<< " 音程:"
						<< (givedIntervalAsStr ?
							midireader::toIntervalStr(n.interval, isYamaha) : std::to_string(n.interval))
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
		for (size_t lane = 0; lane < 4; lane++) {
			cout << std::setfill(' ') << std::setw(4) << lane << '|';
		}
		cout << '\n';
		cout << "hit |";
		for (auto i : intervalNumbers) {
			cout << std::setfill(' ') << std::setw(4) << toscore.numofHitNotes(i) << '|';
		}
		cout << '\n';
		cout << "hold|";
		for (auto i : intervalNumbers) {
			cout << std::setfill(' ') << std::setw(4) << toscore.numofHoldNotes(i) << '|';
		}
		cout << '\n';

		size_t cnt = 0;
		cout << "all |";
		for (auto i : intervalNumbers) {
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