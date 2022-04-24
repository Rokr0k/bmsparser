#include "bmsparser/bmsparser.h"
#include <fstream>
#include <regex>
#include <stack>
#include <map>
#include <algorithm>
#include <exception>

using namespace bms;

const static std::regex randomRegex(R"(^\s*#RANDOM\s*(\d+)\s*$)", std::regex_constants::icase);
const static std::regex ifRegex(R"(^\s*#IF\s*(\d+)\s*$)", std::regex_constants::icase);
const static std::regex elseRegex(R"(^\s*#ELSE\s*$)", std::regex_constants::icase);
const static std::regex endifRegex(R"(^\s*#ENDIF\s*$)", std::regex_constants::icase);

const static std::regex genreRegex(R"(^\s*#GENRE\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex titleRegex(R"(^\s*#TITLE\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex nestedSubtitleRegex(R"(^(.*)\s*[\(\[\uFF50\<\"\-](.*)[\)\]\uFF50\>\"\-]$)", std::regex_constants::icase);
const static std::regex artistRegex(R"(^\s*#ARTIST\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex subtitleRegex(R"(^\s*#SUBTITLE\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex subartistRegex(R"(^\s*#SUBARTIST\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex stagefileRegex(R"(^\s*#STAGEFILE\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex bannerRegex(R"(^\s*#BANNER\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex playLevelRegex(R"(^\s*#PLAYLEVEL\s*(\d+)\s*$)", std::regex_constants::icase);
const static std::regex totalRegex(R"(^\s*#TOTAL\s*(\d+(\.\d+)?)\s*$)", std::regex_constants::icase);
const static std::regex rankRegex(R"(^\s*#RANK\s*(\d+)\s*$)", std::regex_constants::icase);
const static std::regex wavsRegex(R"(^\s*#WAV([0-9A-Z]{2})\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex bmpsRegex(R"(^\s*#BMP([0-9A-Z]{2})\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex lnobjRegex(R"(^\s*#LNOBJ\s*([0-9A-Z]{2})\s*$)", std::regex_constants::icase);
const static std::regex bpmRegex(R"(^\s*#BPM\s*(\d+(\.\d+)?(E\+\d+)?)\s*$)", std::regex_constants::icase);
const static std::regex bpmsRegex(R"(^\s*#BPM([0-9A-Z]{2})\s*(\d+(\.\d+)?(E\+\d+)?)\s*$)", std::regex_constants::icase);
const static std::regex stopsRegex(R"(^\s*#STOP([0-9A-Z]{2})\s*(\d+)\s*$)", std::regex_constants::icase);
const static std::regex signatureRegex(R"(^\s*#(\d{3})02:(\d+(\.\d+)?(E\+\d+)?)\s*$)", std::regex_constants::icase);
const static std::regex notesRegex(R"(^\s*#(\d{3})([0-9A-Z]{2}):(.*)\s*$)", std::regex_constants::icase);

const static std::regex filenameParser(R"(^(.*)(\.\S+)$)");

static bool file_check(const std::string &filename);
static std::string file_cascade(const std::string &filename);

static float fraction_diff(float* signatures, float a, float b);

Chart::Chart(const std::string& filename)
{
    if(!file_check(filename))
    {
        throw std::invalid_argument("filename");
    }

    std::string parent(filename.substr(0, filename.find_last_of('/')+1));

    this->player = 1;
    this->genre = "";
    this->title = "";
    this->artist = "";
    this->subtitle = "";
    this->stagefile = "";
    this->playLevel = 0;
    this->total = 0;
    this->rank = 2;
    this->wavs = new std::string[1296];
    this->bmps = new std::string[1296];
    this->signatures = new float[1000];
    for (int i = 0; i < 1000; i++)
    {
        this->signatures[i] = 1;
    }
    this->sectors.push_back(Sector(0, 0, 130, true));

    int lnobj = 0;
    std::map<int, bool> ln;
    float* bpms = new float[1296];
    float* stops = new float[1296];
    struct speedcore_t
    {
        float fraction;
        enum class Type
        {
            BPM,
            STP
        } type;
        union
        {
            float bpm;
            float stop;
        };
    };
    std::vector<speedcore_t> speedcore;
    int random;
    std::stack<bool> skip;
    skip.push(false);

    srand((unsigned int)time(NULL));

    std::ifstream input(filename);
    std::string line;
    while (std::getline(input, line))
    {
        std::smatch result;

        if (std::regex_match(line, result, randomRegex))
        {
            random = rand()%std::stoi(result[1].str())+1;
        }
        else if (std::regex_match(line, result, ifRegex))
        {
            skip.push(random == std::stoi(result[1].str()));
        }
        else if (std::regex_match(line, result, elseRegex))
        {
            bool top = skip.top();
            skip.pop();
            skip.push(!top);
        }
        else if (std::regex_match(line, result, endifRegex))
        {
            skip.pop();
        }

        if (skip.top())
        {
            continue;
        }

        if (std::regex_match(line, result, genreRegex))
        {
            this->genre = result[1].str();
        }
        else if (std::regex_match(line, result, titleRegex))
        {
            this->title = result[1].str();
            if (std::regex_match(this->title, result, nestedSubtitleRegex))
            {
                this->title = result[1].str();
                this->subtitle = "[" + result[2].str() + "]";
            }
        }
        else if (std::regex_match(line, result, artistRegex))
        {
            this->artist = result[1].str();
        }
        else if (std::regex_match(line, result, subtitleRegex))
        {
            this->subtitle = result[1].str();
        }
        else if (std::regex_match(line, result, subartistRegex))
        {
            this->subartist = result[1].str();
        }
        else if (std::regex_match(line, result, stagefileRegex))
        {
            this->stagefile = parent + result[1].str();
        }
        else if (std::regex_match(line, result, bannerRegex))
        {
            this->banner = parent + result[1].str();
        }
        else if (std::regex_match(line, result, playLevelRegex))
        {
            this->playLevel = std::stoi(result[1].str());
        }
        else if (std::regex_match(line, result, totalRegex))
        {
            this->total = std::stof(result[1].str());
        }
        else if (std::regex_match(line, result, rankRegex))
        {
            this->rank = std::stoi(result[1].str());
        }
        else if (std::regex_match(line, result, wavsRegex))
        {
            int key = std::stoi(result[1].str(), 0, 36);
            this->wavs[key] = parent + result[2].str();
        }
        else if (std::regex_match(line, result, bmpsRegex))
        {
            int key = std::stoi(result[1].str(), 0, 36);
            this->bmps[key] = parent + result[2].str();
        }
        else if (std::regex_match(line, result, bpmRegex))
        {
            this->sectors[0].bpm = std::stof(result[1].str());
        }
        else if (std::regex_match(line, result, lnobjRegex))
        {
            lnobj = std::stoi(result[1].str());
        }
        else if (std::regex_match(line, result, bpmsRegex))
        {
            int key = std::stoi(result[1].str(), 0, 36);
            bpms[key] = std::stof(result[2].str());
        }
        else if (std::regex_match(line, result, stopsRegex))
        {
            int key = std::stoi(result[1].str(), 0, 36);
            stops[key] = std::stoi(result[2].str()) / 192.0f;
        }
        else if (std::regex_match(line, result, signatureRegex))
        {
            int measure = std::stoi(result[1].str());
            this->signatures[measure] = std::stof(result[2].str());
        }
        else if (std::regex_match(line, result, notesRegex))
        {
            int measure = std::stoi(result[1].str());
            int channel = std::stoi(result[2].str(), 0, 36);
            unsigned long long l = result[3].length() / 2;
            for (unsigned long long i = 0; i < l; i++)
            {
                int key = std::stoi(result[3].str().substr(i * 2, 2), 0, 36);
                int hexKey = std::stoi(result[3].str().substr(i * 2, 2), 0, 16);
                if (key)
                {
                    float fraction = (float)i / l;
                    switch (channel)
                    {
                    case 1: // 01
                        this->notes.push_back(Note(fraction, key));
                        break;
                    case 3: // 03
                        speedcore.push_back(speedcore_t{
                            fraction,
                            speedcore_t::Type::BPM,
                            (float)hexKey,
                            });
                        break;
                    case 4: // 04
                        this->notes.push_back(Note(fraction, key, 0));
                        break;
                    case 6: // 06
                        this->notes.push_back(Note(fraction, key, -1));
                        break;
                    case 7: // 07
                        this->notes.push_back(Note(fraction, key, 1));
                        break;
                    case 8: // 08
                        speedcore.push_back(speedcore_t{
                            fraction,
                            speedcore_t::Type::BPM,
                            bpms[key],
                            });
                        break;
                    case 9: // 09
                        speedcore.push_back(speedcore_t{
                            fraction,
                            speedcore_t::Type::STP,
                            stops[key],
                            });
                        break;
                    case 37: // 11
                    case 38: // 12
                    case 39: // 13
                    case 40: // 14
                    case 41: // 15
                    case 42: // 16
                    case 44: // 18
                    case 45: // 19
                    case 73: // 21
                    case 74: // 22
                    case 75: // 23
                    case 76: // 24
                    case 77: // 25
                    case 78: // 26
                    case 80: // 28
                    case 81: // 29
                        this->notes.push_back(Note(fraction, key, channel / 36, channel % 36, key == lnobj));
                        if (key == lnobj)
                        {
                            this->notes.push_back(Note(fraction, key));
                        }
                        break;
                    case 109: // 31
                    case 110: // 32
                    case 111: // 33
                    case 112: // 34
                    case 113: // 35
                    case 114: // 36
                    case 116: // 38
                    case 117: // 39
                    case 145: // 41
                    case 146: // 42
                    case 147: // 43
                    case 148: // 44
                    case 149: // 45
                    case 150: // 46
                    case 152: // 48
                    case 153: // 49
                        this->notes.push_back(Note(fraction, key, true, channel / 36 - 2, channel % 36));
                        break;
                    case 181: // 51
                    case 182: // 52
                    case 183: // 53
                    case 184: // 54
                    case 185: // 55
                    case 186: // 56
                    case 188: // 58
                    case 189: // 59
                    case 217: // 61
                    case 218: // 62
                    case 219: // 63
                    case 220: // 64
                    case 221: // 65
                    case 222: // 66
                    case 224: // 68
                    case 225: // 69
                        this->notes.push_back(Note(fraction, key, channel / 36 - 4, channel % 36, ln[channel]));
                        ln.insert(std::make_pair(channel, !ln[channel]));
                        break;
                    case 469: // D1
                    case 470: // D2
                    case 471: // D3
                    case 472: // D4
                    case 473: // D5
                    case 474: // D6
                    case 476: // D8
                    case 477: // D9
                    case 505: // E1
                    case 506: // E2
                    case 507: // E3
                    case 508: // E4
                    case 509: // E5
                    case 510: // E6
                    case 512: // E8
                    case 513: // E9
                        this->notes.push_back(Note(fraction, key, false, channel / 36 - 12, channel % 36));
                        break;
                    }
                }
            }
        }
    }

    std::stable_sort(speedcore.begin(), speedcore.end(), [](const speedcore_t& a, const speedcore_t& b)
        { return a.fraction < b.fraction; });
    for (speedcore_t& core : speedcore)
    {
        const Sector& last = *std::find_if(this->sectors.rbegin(), this->sectors.rend(), [&core](const Sector& a)
            { return a.fraction < core.fraction || (a.inclusive && a.fraction == core.fraction); });
        float time = last.time + (last.bpm > 0 ? fraction_diff(this->signatures, last.fraction, core.fraction) * 240 / last.bpm : 0);
        switch (core.type)
        {
        case speedcore_t::Type::BPM:
            this->sectors.push_back(Sector(core.fraction, time, core.bpm, true));
            break;
        case speedcore_t::Type::STP:
            this->sectors.push_back(Sector(core.fraction, time, 0, true));
            this->sectors.push_back(Sector(core.fraction, time + (last.bpm > 0 ? core.stop * 240 / last.bpm : 0), last.bpm, false));
            break;
        }
    }
    std::stable_sort(this->notes.begin(), this->notes.end(), [](const Note& a, const Note& b)
        { return a.fraction < b.fraction; });
    for (Note& note : this->notes)
    {
        const Sector& sect = *std::find_if(this->sectors.rbegin(), this->sectors.rend(), [&note](const Sector& a)
            { return a.fraction < note.fraction || (a.inclusive && a.fraction == note.fraction); });
        note.time = sect.time + (sect.bpm > 0 ? fraction_diff(this->signatures, sect.fraction, note.fraction) * 240 / sect.bpm : 0);
    }

    delete[] bpms;
    delete[] stops;
}

Chart::~Chart()
{
    delete[] this->wavs;
    delete[] this->bmps;
    delete[] this->signatures;
}

static bool file_check(const std::string &file)
{
    std::ifstream stream(file);
    return stream.good();
}

static std::string file_cascade(const std::string &file)
{
    if(file_check(file))
    {
        return file;
    }
    std::vector<std::string> audios({".wav", ".ogg", ".mp3"});
    std::vector<std::string> images({".bmp", ".png", ".jpg"});
    std::vector<std::string> videos({".mpg", ".mp4", ".webm"});
    std::smatch result;
    if(std::regex_match(file, result, filenameParser))
    {
        std::string filename = result[1].str();
        std::string extension = result[2].str();
        if(std::find(audios.begin(), audios.end(), extension) != audios.end())
        {
            auto &extension = std::find_if(audios.begin(), audios.end(), [&filename](const std::string &a) {return file_check(filename+a);});
            if(extension != audios.end())
            {
                return filename + *extension;
            }
            else
            {
                return "";
            }
        }
        else if(std::find(images.begin(), images.end(), extension) != images.end())
        {
            auto &extension = std::find_if(images.begin(), images.end(), [&filename](const std::string &a) {return file_check(filename+a);});
            if(extension != images.end())
            {
                return filename + *extension;
            }
            else
            {
                return "";
            }
        }
        else if(std::find(videos.begin(), videos.end(), extension) != videos.end())
        {
            auto &extension = std::find_if(videos.begin(), videos.end(), [&filename](const std::string &a) {return file_check(filename+a);});
            if(extension != videos.end())
            {
                return filename + *extension;
            }
            else
            {
                return "";
            }
        }
        else
        {
            return "";
        }
    }
    else
    {
        return "";
    }
}

static float fraction_diff(float* signatures, float a, float b)
{
    bool negative = a > b;
    if (negative)
    {
        std::swap(a, b);
    }
    int aM = (int)a;
    int bM = (int)b;
    float aF = a - aM;
    float bF = b - bM;
    float result;
    if (aM == bM)
    {
        result = (bF - aF) * signatures[aM];
    }
    else
    {
        result = (1 - aF) * signatures[aM] + bF * signatures[bM];
        for (int i = aM + 1; i < bM; i++)
        {
            result += signatures[i];
        }
    }
    if (negative)
    {
        result = -result;
    }
    return result;
}

float Chart::resolveSignatures(float fraction)
{
    return fraction_diff(this->signatures, 0, fraction);
}

float Chart::timeToFraction(float time)
{
    const Sector &last = *std::find_if(this->sectors.rbegin(), this->sectors.rend(), [&time](const Sector &a) {return a.time < time || (a.inclusive && a.time == time);});
    return last.fraction + (time - last.time) * last.bpm / 240;
}