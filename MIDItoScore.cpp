#include "MIDItoScore.hpp"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>

namespace miditoscore {


    MIDItoScore::MIDItoScore() {}

    MIDItoScore::~MIDItoScore() {}


    int MIDItoScore::writeScore(const std::string & fileName, const NoteFormat & format, const std::vector<midireader::NoteEvent> &notes) {
        std::ofstream scoreFile(fileName.c_str(), std::ios::app);
        if (!scoreFile.is_open())
            return Status::E_CANNOT_OPEN_FILE;

        return writeScore(scoreFile, format, notes);
    }

    int MIDItoScore::writeScore(std::ostream & stream, const NoteFormat & format, const std::vector<midireader::NoteEvent> &notes) {
        using namespace midireader;

        int ret = Status::S_OK;

        clear();

        noteFormat = format;
        noteAggregate.resize(format.laneAllocation.size());
        std::vector<std::vector<NoteEvent>> laneNotes(format.laneAllocation.size());

        // group by lane number and handle invalid notes
        int64_t prevTime = notes.front().time;
        size_t counter = 1;
        for (const auto& note : notes) {
            int laneIndex = selectNoteLane(format, note);
            if (laneIndex >= 0) {
                laneNotes.at(laneIndex).push_back(note);
            }

            if (note.type == midireader::MidiEvent::NoteOn) {
                // enumerize invalid notes
                if (laneIndex < 0) {
                    deviatedNotes.push_back(note);
                    ret |= Status::S_EXIST_DEVIATEDNOTES;
                }

                // check number of lanes where exists parallel notes
                if (format.parallelsLimit.has_value()) {
                    if (prevTime != note.time) {
                        counter = 1;
                    } else {
                        counter++;
                        if (counter > format.parallelsLimit) {
                            parallelNotes.push_back(note);
                            ret |= Status::E_MANY_PARALLELS;
                        }
                    }

                    prevTime = note.time;
                }

                // add exsiting channels
                if (std::find(channels.cbegin(), channels.cend(), note.channel) != channels.cend()) {
                    channels.push_back(note.channel);
                }
            }
        }

        std::sort(channels.begin(), channels.end(), std::less<int>());

        std::string scoreString;
        size_t currentBar = 1;
        std::vector<noteevent_const_itr_t> beginIterators(format.laneAllocation.size());
        std::vector<noteevent_const_itr_t> endIterators(format.laneAllocation.size());
        for (size_t i = 0; i < format.laneAllocation.size(); i++) {
            beginIterators.at(i) = laneNotes.at(i).cbegin();
            endIterators.at(i) = laneNotes.at(i).cbegin();
        }
        std::vector<bool> holdStarted(format.laneAllocation.size(), false);

        while (true) {
            for (size_t lane = 0; lane < format.laneAllocation.size(); lane++) {
                auto& beginIt = beginIterators.at(lane);
                auto& endIt = endIterators.at(lane);

                // calculate range of current bar
                for (endIt = beginIt; endIt != laneNotes.at(lane).cend(); endIt++) {
                    if (endIt->bar != currentBar) break;
                };

                // enumerate useable events to write score
                std::vector<ScoreNote> scoreNotes;
                for (auto it = beginIt; it != endIt; it++) {
                    if (it == beginIt && holdStarted.at(lane)) {
                        scoreNotes.emplace_back(NoteType::HOLD_END, &*it);
                        holdStarted.at(lane) = false;

                        // aggregate
                        noteAggregate.at(lane).increment(scoreNotes.back().type);
                    } else if (it->type == MidiEvent::NoteOn) {
                        // calculate note length
                        const math::Fraction end = (it + 1)->bar + (it + 1)->posInBar;
                        const math::Fraction beg = it->bar + it->posInBar;
                        const math::Fraction length = end - beg;

                        const bool isHold = (length >= format.holdMinLength);

                        if (isHold) {
                            scoreNotes.emplace_back(NoteType::HOLD_BEGIN, &*it);
                            if ((it + 1)->bar == currentBar) {
                                // hold end is in current bar
                                scoreNotes.emplace_back(NoteType::HOLD_END, &*(it+1));
                            } else {
                                // hold end is out of current bar
                                holdStarted.at(lane) = true;
                            }
                        } else {
                            NoteType type = NoteType::HIT;
                            if (format.exNoteDecider) {
                                if (format.exNoteDecider(&*it))
                                    type = NoteType::EX_HIT;
                                else
                                    type = NoteType::HIT;
                            }
                            scoreNotes.emplace_back(type, &*it);
                        }

                        // aggregate
                        noteAggregate.at(lane).increment(scoreNotes.back().type);
                    }
                }

                if (scoreNotes.size() > 0) {
                    ret |= createScoreString(scoreNotes, scoreString);

                    if (scoreString.size() > format.allowedLineLength) {
                        longLines.emplace_back(currentBar, format.laneAllocation.at(lane));
                        ret |= Status::E_EXIST_LONGLINES;
                    }

                    // write the score data to file.
                    using namespace std;
                    stream << lane << ':'
                        << setfill('0') << setw(3) << currentBar << ':'
                        << scoreString << endl;
                }

                // ready for next bar
                beginIt = endIt;
            }

            // check loop condition
            size_t numofEndedLanes = 0;
            for (size_t lane = 0; lane < format.laneAllocation.size(); lane++) {
                if (beginIterators.at(lane) == laneNotes.at(lane).cend()) numofEndedLanes++;
            }
            if (numofEndedLanes == format.laneAllocation.size()) {
                break;
            }

            currentBar++;
        }


        return ret;
    }

    int MIDItoScore::createScoreString(const std::vector<ScoreNote>& scoreNotes, std::string& scoreString) {
        int ret = Status::S_OK;

        // calculate line length of score data
        size_t mininalUnit = 1;
        for (auto it = scoreNotes.cbegin(); it != scoreNotes.cend(); it++) {
            mininalUnit = std::lcm(mininalUnit, it->evt->posInBar.get().d);
        }

        // create empty score data
        scoreString.resize(mininalUnit);
        for (auto it = scoreString.begin(); it != scoreString.end(); it++) *it = '0';

        // add note to the score data
        for (auto it = scoreNotes.cbegin(); it != scoreNotes.cend(); it++) {
            // calculate note offset
            math::Fraction notePos(it->evt->posInBar);
            math::Fraction unit(1, mininalUnit);
            math::adjustDenom(notePos, unit);
            size_t offset = notePos.get().n;

            // check concurrent notes
            if (scoreString.at(offset) != '0') {
                concurrentNotes.push_back(*(it->evt));
                ret |= Status::E_EXIST_CONCURRENTNOTES;
            }

            // write to buffer
            scoreString.at(offset) = '0' + static_cast<int>(it->type) + (it->evt->channel << 3);
        }

        return ret;
    }

    MIDItoScore::NoteAggregate MIDItoScore::getNoteAggregate(int interval) const {
        int pos = -1;

        for (size_t i = 0; i < noteFormat.laneAllocation.size(); i++) {
            if (noteFormat.laneAllocation.at(i) == interval) {
                pos = i;
            }
        }

        if (pos < 0)
            abort();

        return noteAggregate.at(pos);
    }

    int MIDItoScore::selectNoteLane(const NoteFormat &format, const midireader::NoteEvent &note) {
        auto lane_it = std::find(format.laneAllocation.begin(), format.laneAllocation.end(), note.interval);
        if (lane_it == format.laneAllocation.cend())
            return -1;

        return static_cast<int>(lane_it - format.laneAllocation.cbegin());
    }

    void MIDItoScore::clear() {
        concurrentNotes.clear();
        deviatedNotes.clear();
        noteAggregate.clear();
        parallelNotes.clear();
    }

    bool Success(int s) { return s >= 0; };
    bool Failed(int s) { return s < 0; };


}
