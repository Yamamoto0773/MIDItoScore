#include "MIDItoScore.hpp"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <array>
#include <filesystem>

// #define WII_VERSION

#ifndef WII_VERSION
#define BUTTON_VERSION
#endif


bool isNumber(char ch) {
    return '0' <= ch && ch <= '9';
}

bool isIntervalAlphabet(char ch) {
    return ('a' <= ch && ch <= 'g') || ('A' <= ch && ch <= 'G');
}

int searchTrack(const std::vector<midireader::Track>& tracks, const std::string searchName) {
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

bool toNumber(const std::string& str, int* number = nullptr) {
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

bool toDouble(const std::string& str, double* number = nullptr) {
    size_t numEndedPos;
    double n;
    try {
        n = std::stod(str, &numEndedPos);
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


bool toIntervalStr(std::string& str, std::string& intervalStr) {
    intervalStr.clear();

    if (str.length() < 2)
        return false;

    if (!isIntervalAlphabet(str.at(0)))
        return false;

    constexpr std::array<char, 5> sharpAttachable = { 'C', 'D', 'F', 'G', 'A' };
    const bool isAttachable =
        std::find_if(
            sharpAttachable.cbegin(),
            sharpAttachable.cend(),
            [&](char ch) { return ch == std::toupper(str.at(0)); }
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

void stop() {
    std::cout << "\nプログラムを終了するにはEnterを押してください\n";
    std::cin.ignore();
}

int main() {
    using namespace midireader;
    namespace fs = std::filesystem;
    using std::cin;
    using std::cout;
    using std::string;
    using std::wstring;

    bool loopFlag = true;

    // get class number
    cout << "曲IDを入力して下さい\n";
    fs::path musicIDPath;
    while (loopFlag) {
        cout << ">";

        string input;
        std::getline(cin, input);

        int n;
        if (toNumber(input, &n) && n >= 0) {
            musicIDPath = std::to_string(n);
            break;
        } else {
            cout << "[!] 0以上の半角数字で入力してください\n";
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
        stop();
    case midireader::Status::E_INVALID_FILE:
        cout << "[!] MIDIファイルが破損しています\n";
        stop();
    case midireader::Status::S_OK:
        cout << "読み込み完了\n\n";
        break;
    case midireader::Status::S_NO_EMBED_TIMESIGNATURE:
        cout << "[!] MIDIファイルに拍子情報が埋め込まれていません.\n";
        stop();
    default:
        break;
    }

    if (midir.getTempoEvent().empty()) {
        cout << "[!] MIDIファイルにテンポ情報が埋め込まれていません.\n";
        stop();
    }

    // get interval
    cout << "打ち込みに使った音程を入力してください\n"
        << "(例: 音名で入力する場合:C3, D#3  番号で入力する場合:60, 63)\n";

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

    PitchNotation pitchNotation;
    if (givedIntervalAsStr) {
        cout << "DAWの一番低い音程を入力してください [C-2, C-1, C0のどれか]\n";

        while (true) {
            cout << ">";

            std::string inputStr, intervalStr;
            std::getline(cin, inputStr);

            if (toIntervalStr(inputStr, intervalStr)) {
                intervalStr.front() = toupper(intervalStr.front());
                if (intervalStr == "C-2") {
                    pitchNotation = PitchNotation::A3_440Hz;
                    break;
                } else if (intervalStr == "C-1") {
                    pitchNotation = PitchNotation::A4_440Hz;
                    break;
                } else if (intervalStr == "C0") {
                    pitchNotation = PitchNotation::A5_440Hz;
                    break;
                } else {
                    cout << "[!] C-2, C-1, C0のどれかを入力してください．\n";
                }
            } else {
                cout << "[!] 音程として正しくありません．\n";
            }
        }

        // convert interval string to number
        for (size_t i = 0; i < 4; i++) {
            if (intervalNumbers.at(i) == -1) {
                intervalNumbers.at(i) = midireader::toNoteNum(intervalStrings.at(i), pitchNotation);
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

#ifdef BUTTON_VERSION
    // get chorus timing
    cout << "\n選曲時に，曲をプレビューするときの再生位置を入力してください (例: 12.3)\n"
        << "ループ再生したときに，なるべく違和感のないようにお願いします\n";

    double chorusBegSec;
    double chorusEndSec;
    for (int i = 0; i < 2; i++) {
        while (true) {
            if (i == 0)
                cout << "再生開始位置 [s] >";
            else
                cout << "再生終了位置 [s] >";

            string input;
            std::getline(cin, input);

            double d;
            if (toDouble(input, &d) && d >= 0) {
                if (i == 0)
                    chorusBegSec = d;
                else
                    chorusEndSec = d;

                break;
            } else {
                cout << "[!] 0以上の小数を入力してください\n";
            }
        }

        if (i == 1) {
            if (chorusBegSec >= chorusEndSec) {
                cout << "[!] 無効な範囲です\n";
                i = -1; // loop counter reset
                continue;
            }
        }
    }
#endif // BUTTON_VERSION

    // set note format
    miditoscore::MIDItoScore toscore;
    miditoscore::NoteFormat format;
    format.holdMinLength = holdMinLen;
    format.laneAllocation = intervalNumbers;
    format.allowedLineLength = 1024;
    format.exNoteDecider = [] (const midireader::NoteEvent* note) { return note->velocity >= 110; };
#ifdef WII_VERSION
    format.parallelsLimit = 2;
#endif // WII_VERSION


    // ---------------------------------------------------
    // write score

    cout << "\n譜面データを作成します\n";

#ifdef BUTTON_VERSION
    // create empty directory
    while (true) {
        if (!fs::exists(musicIDPath)) {
            try {
                fs::create_directory(musicIDPath);
            } catch (std::exception e) {
                cout << "[!] ディレクトリ作成に失敗しました\n";
                stop();
            }
            break;
        } else {
            try {
                fs::remove_all(musicIDPath);
            } catch (std::exception e) {
                cout << "[!] ディレクトリの削除に失敗しました\n";
                stop();
            }
        }
    }

    const auto scoreFilePath = musicIDPath / "score.txt";
#else
    const auto scoreFilePath = musicIDPath.generic_string() + "_wii.txt";
#endif // BUTTON_VERSION


    std::ofstream score;
    score.open(scoreFilePath);

    score << u8"begin:header\n\n";
    score << u8"id:" << musicIDPath.u8string() << '\n';

#ifdef BUTTON_VERSION
    score << u8"title:曲名" << '\n';
    score << u8"artist:アーティスト名" << '\n';
    score << std::fixed << std::setprecision(3);
    score << u8"chobeg:" << chorusBegSec << "\n";
    score << u8"choend:" << chorusEndSec << "\n";
#endif

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

    score << u8"\nend\n\n";

#ifdef WII_VERSION
    score << u8"begin:header-wii\n\n";
    score << u8"level:0:0:0" << "\n";
    score << u8"\nend\n\n";
#endif

#ifdef BUTTON_VERSION
    char firstTrackName = '1';
#else
    char firstTrackName = '4';
#endif

    // write note position
    for (char targetTrackName = firstTrackName; targetTrackName < firstTrackName + 3; targetTrackName++) {

        int trackNum = -1;
        const auto tracks = midir.getTracks();
        trackNum = searchTrack(tracks, std::string(1, targetTrackName));
        if (trackNum < 0) {
            continue;
        }

        cout << '\n';
        switch (targetTrackName) {
#ifdef BUTTON_VERSION
        case '1':
            cout << "easy譜面を作成中です... ";
            score << "begin:easy" << "\n\n";
            break;
        case '2':
            cout << "normal譜面を作成中です... ";
            score << "begin:normal" << "\n\n";
            break;
        case '3':
            cout << "hard譜面を作成中です... ";
            score << "begin:hard" << "\n\n";
            break;
#else
        case '4':
            cout << "easy(wii)譜面を作成中です... ";
            score << "begin:easy-wii" << "\n\n";
            break;
        case '5':
            cout << "normal(wii)譜面を作成中です... ";
            score << "begin:normal-wii" << "\n\n";
            break;
        case '6':
            cout << "hard(wii)譜面を作成中です... ";
            score << "begin:hard-wii" << "\n\n";
            break;
#endif
        }

        auto ret = toscore.writeScore(score, format, midir.getNoteEvent(trackNum));

        score << "\nend\n\n";

        // print return value
        if (ret == miditoscore::Status::S_OK)
            cout << "完了\n";
        else {
            cout << "エラー\n";

            if (isInclude(ret, miditoscore::Status::E_EXIST_CONCURRENTNOTES)) {
                cout << "[!] 同じタイミングのノーツが存在しています．\n";
                cout << "  ->長押しの後にあるノーツと繋がっていないかチェックしてください\n";
                cout << "  ->ノーツが重なっていないかチェックしてください\n";
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
                            midireader::toNoteName(n.interval, pitchNotation) : std::to_string(n.interval))
                        << '\n';

                    if (++cnt >= 10)
                        break;
                }

                if (notes.size() > 10)
                    cout << "...他" << notes.size() - 10 << "コ\n";

                cout << '\n';
            }
            if (isInclude(ret, miditoscore::Status::E_MANY_PARALLELS)) {
                cout << "[!] 3レーン以上の同時ノーツが存在しています．\n";
                cout << "-- 問題のあるノーツ --\n";

                int cnt = 0;
                const auto notes = toscore.getParallelNotes();
                for (auto n : notes) {
                    using namespace std;
                    cout << "小節:"
                        << setfill('0') << setw(3) << n.bar
                        << " 小節内位置:"
                        << n.posInBar.get_str()
                        << " 音程:"
                        << (givedIntervalAsStr ?
                            midireader::toNoteName(n.interval, pitchNotation) : std::to_string(n.interval))
                        << '\n';

                    if (++cnt >= 10)
                        break;
                }

                if (notes.size() > 10)
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
                            midireader::toNoteName(n.interval, pitchNotation) : std::to_string(n.interval))
                        << '\n';

                    if (++cnt >= 10)
                        break;
                }

                if (notes.size() > 10)
                    cout << "...他" << notes.size() - 10 << "コ\n";

                cout << '\n';
            }
            if (isInclude(ret, miditoscore::Status::E_EXIST_LONGLINES)) {
                cout << "[!] 譜面データの1行がとても長くなっています.\n";
                cout << "  -> 許容できる1行の文字数は" << format.allowedLineLength << "です\n";
                cout << "  -> 該当する箇所のノーツをDAW上でクォンタイズしてください．\n";
                cout << "-- 該当する箇所 --\n";

                int cnt = 0;
                auto lines = toscore.getLongLines();
                for (auto l : lines) {
                    using namespace std;
                    cout << "小節:"
                        << setfill('0') << setw(3) << l.bar
                        << " 音程:"
                        << (givedIntervalAsStr ?
                            midireader::toNoteName(l.interval, pitchNotation) : std::to_string(l.interval))
                        << '\n';

                    if (++cnt >= 10)
                        break;
                }

                if (lines.size() > 10)
                    cout << "...他" << lines.size() - 10 << "コ\n";

                cout << '\n';
            }
        }

        cout << "--ノーツ内訳-----\n";
        cout << "lane |";
        for (size_t lane = 0; lane < 4; lane++) {
            cout << std::setfill(' ') << std::setw(4) << lane << '|';
        }
        cout << '\n';
        cout << "hit  |";
        for (auto i : intervalNumbers) {
            cout << std::setfill(' ') << std::setw(4) << toscore.getNoteAggregate(i).hit << '|';
        }
        cout << '\n';
        cout << "exhit|";
        for (auto i : intervalNumbers) {
            cout << std::setfill(' ') << std::setw(4) << toscore.getNoteAggregate(i).exhit << '|';
        }
        cout << '\n';
        cout << "hold |";
        for (auto i : intervalNumbers) {
            cout << std::setfill(' ') << std::setw(4) << toscore.getNoteAggregate(i).hold << '|';
        }
        cout << '\n';

        size_t totalNoteCnt = 0;
        cout << "all  |";
        for (auto i : intervalNumbers) {
            cout << std::setfill(' ') << std::setw(4) << toscore.getNoteAggregate(i).total() << '|';
            totalNoteCnt += toscore.getNoteAggregate(i).total();
        }
        cout << '\n';
        cout << "total:" << totalNoteCnt << '\n';
    }

    score.close();
    midir.close();


#ifdef BUTTON_VERSION
    // ----------------------------------
    // create ini file

    // get music file path, copy music file
    cout << "\n書き出した音源へのパスを入力して下さい(\"や\'がついたままでもOKです)\n";
    fs::path musicFilePath;
    while (true) {
        cout << ">";
        std::string input;
        std::getline(std::cin, input);

        // erase ' or "
        if (!input.empty()) {
            if (input.front() == '\'' || input.front() == '\"')
                input.erase(input.begin());
            if (input.back() == '\'' || input.back() == '\"')
                input.erase(input.end() - 1);
        }

        try {
            musicFilePath = input;

            fs::copy_file(
                musicFilePath,
                musicIDPath / musicFilePath.filename(),
                fs::copy_options::overwrite_existing
            );
        } catch (std::exception e) {
            cout << "[i] " << e.what() << "\n";
            continue;
        }
        break;
    }

    // get image file path, copy image file
    cout << "\nジャケット写真へのパスを入力して下さい(\"や\'がついたままでもOKです)\n";
    fs::path imageFilePath;
    while (true) {
        cout << ">";
        std::string input;
        std::getline(cin, input);

        // erase ' or "
        if (!input.empty()) {
            if (input.front() == '\'' || input.front() == '\"')
                input.erase(input.begin());
            if (input.back() == '\'' || input.back() == '\"')
                input.erase(input.end() - 1);
        }

        try {
            imageFilePath = fs::path(input);

            fs::copy_file(
                imageFilePath,
                musicIDPath / imageFilePath.filename(),
                fs::copy_options::overwrite_existing
            );
        } catch (std::exception e) {
            cout << "[i] " << e.what() << "\n";
            continue;
        }
        break;
    }

    // write to ini file
    std::basic_ofstream<char32_t> ini;
    ini.open(musicIDPath / "score.ini");

    ini << U"jacket=\"" << imageFilePath.filename().u32string() << U"\"\n";
    ini << U"music=\"" << musicFilePath.filename().u32string() << U"\"\n";
    ini << U"score=\"" << U"score.txt" << U"\"\n";
    ini << U"musicEx=\"" << U"\"\n";

    ini.close();

    cout << "\n譜面データのディレクトリを作成しました\n";
#endif // BUTTON_VERSION

    stop();

    return 0;
}
