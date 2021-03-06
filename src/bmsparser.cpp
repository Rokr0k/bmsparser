#include "bmsparser/bmsparser.hpp"
#include <fstream>
#include <regex>
#include <stack>
#include <map>
#include <algorithm>

using namespace bms;

const static std::regex randomRegex(R"(^\s*#RANDOM\s*(\d+)\s*$)", std::regex_constants::icase);
const static std::regex ifRegex(R"(^\s*#IF\s*(\d+)\s*$)", std::regex_constants::icase);
const static std::regex elseRegex(R"(^\s*#ELSE\s*$)", std::regex_constants::icase);
const static std::regex endifRegex(R"(^\s*#ENDIF\s*$)", std::regex_constants::icase);

const static std::regex genreRegex(R"(^\s*#GENRE\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex titleRegex(R"(^\s*#TITLE\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex nestedSubtitleRegex(R"(^(.*)\s*[\(\[\<\-](.*)[\)\]\>\-]$)", std::regex_constants::icase);
const static std::regex artistRegex(R"(^\s*#ARTIST\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex subtitleRegex(R"(^\s*#SUBTITLE\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex subartistRegex(R"(^\s*#SUBARTIST\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex stagefileRegex(R"(^\s*#STAGEFILE\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex bannerRegex(R"(^\s*#BANNER\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex playLevelRegex(R"(^\s*#PLAYLEVEL\s*(\d+)\s*$)", std::regex_constants::icase);
const static std::regex difficultyRegex(R"(^\s*#DIFFICULTY\s*([12345])\s*$)", std::regex_constants::icase);
const static std::regex totalRegex(R"(^\s*#TOTAL\s*(\d+(\.\d+)?)\s*$)", std::regex_constants::icase);
const static std::regex rankRegex(R"(^\s*#RANK\s*([0123])\s*$)", std::regex_constants::icase);
const static std::regex wavsRegex(R"(^\s*#WAV([0-9A-Z]{2})\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex bmpsRegex(R"(^\s*#BMP([0-9A-Z]{2})\s*(.*)\s*$)", std::regex_constants::icase);
const static std::regex lnobjRegex(R"(^\s*#LNOBJ\s*([0-9A-Z]{2})\s*$)", std::regex_constants::icase);
const static std::regex bpmRegex(R"(^\s*#BPM\s*(\d+(\.\d+)?(E\+\d+)?)\s*$)", std::regex_constants::icase);
const static std::regex bpmsRegex(R"(^\s*#BPM([0-9A-Z]{2})\s*(\d+(\.\d+)?(E\+\d+)?)\s*$)", std::regex_constants::icase);
const static std::regex stopsRegex(R"(^\s*#STOP([0-9A-Z]{2})\s*(\d+)\s*$)", std::regex_constants::icase);
const static std::regex signatureRegex(R"(^\s*#(\d{3})02:(\d+(\.\d+)?(E\+\d+)?)\s*$)", std::regex_constants::icase);
const static std::regex notesRegex(R"(^\s*#(\d{3})([0-9A-Z]{2}):(.*)\s*$)", std::regex_constants::icase);

static bool file_check(const std::string &file);

static float fraction_diff(float *signatures, float a, float b);

static Obj create_bgm(float fraction, int key);
static Obj create_bmp(float fraction, int key, int layer);
static Obj create_note(float fraction, int player, int line, int key, bool end);
static Obj create_inv(float fraction, int player, int line, int key);
static Obj create_bomb(float fraction, int player, int line, int damage);

Chart::Chart()
{
    this->type = Type::Single;
    this->genre = "";
    this->title = "";
    this->artist = "";
    this->subtitle = "";
    this->stagefile = "";
    this->banner = "";
    this->playLevel = 0;
    this->difficulty = 2;
    this->total = 160;
    this->rank = 2;
    this->wavs = new std::string[1296];
    this->bmps = new std::string[1296];
    this->signatures = new float[1000];
    for (int i = 0; i < 1000; i++)
    {
        this->signatures[i] = 1;
    }
    this->sectors.push_back(Sector(0, 0, 130, true));
}

Chart::Chart(const Chart &chart)
{
    this->type = chart.type;
    this->genre = chart.genre;
    this->title = chart.title;
    this->artist = chart.artist;
    this->subtitle = chart.subtitle;
    this->stagefile = chart.stagefile;
    this->banner = chart.banner;
    this->playLevel = chart.playLevel;
    this->difficulty = chart.difficulty;
    this->total = chart.total;
    this->rank = chart.rank;
    this->wavs = new std::string[1296];
    this->bmps = new std::string[1296];
    for (int i = 0; i < 1296; i++)
    {
        this->wavs[i] = chart.wavs[i];
        this->bmps[i] = chart.bmps[i];
    }
    this->signatures = new float[1000];
    for (int i = 0; i < 1000; i++)
    {
        this->signatures[i] = chart.signatures[i];
    }
    this->objs.assign(chart.objs.begin(), chart.objs.end());
    this->sectors.assign(chart.sectors.begin(), chart.sectors.end());
}

Chart::~Chart()
{
    delete[] this->wavs;
    delete[] this->bmps;
    delete[] this->signatures;
}

Chart *bms::parseBMS(const std::string &file)
{
    Chart *chart = new Chart;

    std::string parent = file.substr(0, file.find_last_of('/') + 1);

    std::vector<int> lnobj;
    std::map<int, bool> ln;
    float *bpms = new float[1296];
    float *stops = new float[1296];
    struct speedcore_t
    {
        float fraction;
        enum class Type
        {
            BPM,
            STP
        } type;
        float value;
    };
    std::vector<speedcore_t> speedcore;
    int random;
    std::stack<bool> skip;
    skip.push(false);

    srand((unsigned int)time(NULL));

    std::ifstream input(file);
    std::string line;

    while (std::getline(input, line))
    {
        if (line.empty() || line[0] != '#')
        {
            continue;
        }

        std::smatch result;

        if (std::regex_match(line, result, randomRegex))
        {
            random = rand() % std::stoi(result[1].str()) + 1;
        }
        else if (std::regex_match(line, result, ifRegex))
        {
            skip.push(random != std::stoi(result[1].str()));
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
            chart->genre = result[1].str();
        }
        else if (std::regex_match(line, result, titleRegex))
        {
            chart->title = result[1].str();
            if (std::regex_match(chart->title, result, nestedSubtitleRegex))
            {
                chart->title = result[1].str();
                chart->subtitle = "[" + result[2].str() + "]";
            }
        }
        else if (std::regex_match(line, result, artistRegex))
        {
            chart->artist = result[1].str();
        }
        else if (std::regex_match(line, result, subtitleRegex))
        {
            chart->subtitle = result[1].str();
        }
        else if (std::regex_match(line, result, subartistRegex))
        {
            chart->subartist = result[1].str();
        }
        else if (std::regex_match(line, result, stagefileRegex))
        {
            chart->stagefile = parent + result[1].str();
        }
        else if (std::regex_match(line, result, bannerRegex))
        {
            chart->banner = parent + result[1].str();
        }
        else if (std::regex_match(line, result, playLevelRegex))
        {
            chart->playLevel = std::stoi(result[1].str());
        }
        else if (std::regex_match(line, result, difficultyRegex))
        {
            chart->difficulty = std::stoi(result[1].str());
        }
        else if (std::regex_match(line, result, totalRegex))
        {
            chart->total = std::stof(result[1].str());
        }
        else if (std::regex_match(line, result, rankRegex))
        {
            chart->rank = std::stoi(result[1].str());
        }
        else if (std::regex_match(line, result, wavsRegex))
        {
            int key = std::stoi(result[1].str(), nullptr, 36);
            chart->wavs[key] = parent + result[2].str();
        }
        else if (std::regex_match(line, result, bmpsRegex))
        {
            int key = std::stoi(result[1].str(), nullptr, 36);
            chart->bmps[key] = parent + result[2].str();
        }
        else if (std::regex_match(line, result, lnobjRegex))
        {
            lnobj.push_back(std::stoi(result[1].str(), nullptr, 36));
        }
        else if (std::regex_match(line, result, bpmRegex))
        {
            chart->sectors[0].bpm = std::stof(result[1].str());
        }
        else if (std::regex_match(line, result, bpmsRegex))
        {
            int key = std::stoi(result[1].str(), nullptr, 36);
            bpms[key] = std::stof(result[2].str());
        }
        else if (std::regex_match(line, result, stopsRegex))
        {
            int key = std::stoi(result[1].str(), nullptr, 36);
            stops[key] = std::stoi(result[2].str()) / 192.0f;
        }
        else if (std::regex_match(line, result, signatureRegex))
        {
            int measure = std::stoi(result[1].str());
            chart->signatures[measure] = std::stof(result[2].str());
        }
        else if (std::regex_match(line, result, notesRegex))
        {
            int measure = std::stoi(result[1].str());
            int channel = std::stoi(result[2].str(), nullptr, 36);
            unsigned long long l = result[3].length() / 2;
            for (unsigned long long i = 0; i < l; i++)
            {
                int key = std::stoi(result[3].str().substr(i * 2, 2), nullptr, 36);
                if (key)
                {
                    float fraction = measure + (float)i / l;
                    switch (channel)
                    {
                    case 1: // 01
                        chart->objs.push_back(create_bgm(fraction, key));
                        break;
                    case 3: // 03
                        speedcore.push_back(speedcore_t{
                            fraction,
                            speedcore_t::Type::BPM,
                            (float)std::stoi(result[3].str().substr(i * 2, 2), nullptr, 16),
                        });
                        break;
                    case 4: // 04
                        chart->objs.push_back(create_bmp(fraction, key, 0));
                        break;
                    case 6: // 06
                        chart->objs.push_back(create_bmp(fraction, key, -1));
                        break;
                    case 7: // 07
                        chart->objs.push_back(create_bmp(fraction, key, 1));
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
                    case 43: // 17
                    case 44: // 18
                    case 45: // 19
                    case 73: // 21
                    case 74: // 22
                    case 75: // 23
                    case 76: // 24
                    case 77: // 25
                    case 78: // 26
                    case 79: // 27
                    case 80: // 28
                    case 81: // 29
                        if (std::find(lnobj.begin(), lnobj.end(), key) == lnobj.end())
                        {
                            chart->objs.push_back(create_note(fraction, key, channel / 36, channel % 36, false));
                        }
                        else
                        {
                            chart->objs.push_back(create_note(fraction, key, channel / 36, channel % 36, true));
                            chart->objs.push_back(create_bgm(fraction, key));
                        }
                        break;
                    case 109: // 31
                    case 110: // 32
                    case 111: // 33
                    case 112: // 34
                    case 113: // 35
                    case 114: // 36
                    case 115: // 37
                    case 116: // 38
                    case 117: // 39
                    case 145: // 41
                    case 146: // 42
                    case 147: // 43
                    case 148: // 44
                    case 149: // 45
                    case 150: // 46
                    case 151: // 47
                    case 152: // 48
                    case 153: // 49
                        chart->objs.push_back(create_inv(fraction, key, channel / 36 - 2, channel % 36));
                        break;
                    case 181: // 51
                    case 182: // 52
                    case 183: // 53
                    case 184: // 54
                    case 185: // 55
                    case 186: // 56
                    case 187: // 57
                    case 188: // 58
                    case 189: // 59
                    case 217: // 61
                    case 218: // 62
                    case 219: // 63
                    case 220: // 64
                    case 221: // 65
                    case 222: // 66
                    case 223: // 67
                    case 224: // 68
                    case 225: // 69
                        if (ln.find(channel) == ln.end())
                        {
                            ln[channel] = false;
                        }
                        chart->objs.push_back(create_note(fraction, key, channel / 36 - 4, channel % 36, ln[channel]));
                        ln[channel] = !ln[channel];
                        break;
                    case 469: // D1
                    case 470: // D2
                    case 471: // D3
                    case 472: // D4
                    case 473: // D5
                    case 474: // D6
                    case 475: // D7
                    case 476: // D8
                    case 477: // D9
                    case 505: // E1
                    case 506: // E2
                    case 507: // E3
                    case 508: // E4
                    case 509: // E5
                    case 510: // E6
                    case 511: // E7
                    case 512: // E8
                    case 513: // E9
                        chart->objs.push_back(create_bomb(fraction, key, channel / 36 - 12, channel % 36));
                        break;
                    }
                }
            }
        }
    }

    input.close();

    std::stable_sort(speedcore.begin(), speedcore.end(), [](const speedcore_t &a, const speedcore_t &b)
                     { return a.fraction < b.fraction; });
    for (speedcore_t &core : speedcore)
    {
        const Sector last = *std::find_if(chart->sectors.rbegin(), chart->sectors.rend(), [&core](const Sector &a)
                                          { return a.fraction < core.fraction || (a.inclusive && a.fraction == core.fraction); });
        float time = last.time + (last.bpm > 0 ? fraction_diff(chart->signatures, last.fraction, core.fraction) * 240 / last.bpm : 0);
        switch (core.type)
        {
        case speedcore_t::Type::BPM:
            chart->sectors.push_back(Sector(core.fraction, time, core.value, true));
            break;
        case speedcore_t::Type::STP:
            chart->sectors.push_back(Sector(core.fraction, time, 0, true));
            chart->sectors.push_back(Sector(core.fraction, time + (last.bpm > 0 ? core.value * 240 / last.bpm : 0), last.bpm, false));
            break;
        }
    }
    std::stable_sort(chart->objs.begin(), chart->objs.end(), [](const Obj &a, const Obj &b)
                     { return a.fraction < b.fraction; });
    for (Obj &note : chart->objs)
    {
        const Sector &sect = *std::find_if(chart->sectors.rbegin(), chart->sectors.rend(), [&note](const Sector &a)
                                           { return a.fraction < note.fraction || (a.inclusive && a.fraction == note.fraction); });
        note.time = sect.time + (sect.bpm > 0 ? fraction_diff(chart->signatures, sect.fraction, note.fraction) * 240 / sect.bpm : 0);
    }

    delete[] bpms;
    delete[] stops;

    bool p2 = false;
    for (const Obj &obj : chart->objs)
    {
        switch (obj.type)
        {
        case Obj::Type::NOTE:
            if (obj.note.player > 1)
            {
                p2 = true;
            }
            break;
        case Obj::Type::INVISIBLE:
        case Obj::Type::BOMB:
            if (obj.misc.player > 1)
            {
                p2 = true;
            }
            break;
        }
    }

    if (!p2)
    {
        chart->type = Chart::Type::Single;
    }
    else
    {
        chart->type = Chart::Type::Dual;
    }

    return chart;
}

static bool file_check(const std::string &file)
{
    std::ifstream stream(file);
    return stream.good();
}

static float fraction_diff(float *signatures, float a, float b)
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
    float result = bF * (bM >= 0 ? signatures[bM] : 1) - aF * (aM >= 0 ? signatures[aM] : 1);
    for (int i = aM; i < bM; i++)
    {
        if (i >= 0)
        {
            result += signatures[i];
        }
        else
        {
            result += 1;
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
    const std::vector<Sector>::reverse_iterator &i = std::find_if(this->sectors.rbegin(), this->sectors.rend(), [&time](const Sector &a)
                                                                  { return a.time < time || (a.inclusive && a.time == time); });
    if (i != this->sectors.rend())
    {
        return resolveSignatures(i->fraction) + (time - i->time) * i->bpm / 240;
    }
    else
    {
        return resolveSignatures(this->sectors.front().fraction) + (time - this->sectors.front().time) * this->sectors.front().bpm / 240;
    }
}

static Obj create_bgm(float fraction, int key)
{
    Obj obj;
    obj.type = Obj::Type::BGM;
    obj.fraction = fraction;
    obj.bgm.key = key;
    obj.executed = false;
    return obj;
}

static Obj create_bmp(float fraction, int key, int layer)
{
    Obj obj;
    obj.type = Obj::Type::BMP;
    obj.fraction = fraction;
    obj.bmp.key = key;
    obj.bmp.layer = layer;
    obj.executed = false;
    return obj;
}

static Obj create_note(float fraction, int key, int player, int line, bool end)
{
    Obj obj;
    obj.type = Obj::Type::NOTE;
    obj.fraction = fraction;
    obj.note.player = player;
    obj.note.line = line;
    obj.note.key = key;
    obj.note.end = end;
    obj.executed = false;
    return obj;
}

static Obj create_inv(float fraction, int key, int player, int line)
{
    Obj obj;
    obj.type = Obj::Type::INVISIBLE;
    obj.fraction = fraction;
    obj.misc.player = player;
    obj.misc.line = line;
    obj.misc.key = key;
    obj.executed = false;
    return obj;
}

static Obj create_bomb(float fraction, int damage, int player, int line)
{
    Obj obj;
    obj.type = Obj::Type::BOMB;
    obj.fraction = fraction;
    obj.misc.player = player;
    obj.misc.line = line;
    obj.misc.key = damage;
    obj.executed = false;
    return obj;
}
