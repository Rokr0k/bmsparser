#ifndef BMSPARSER_H
#define BMSPARSER_H

#include <string>
#include <vector>

namespace bms
{
    struct Note
    {
        enum class Type
        {
            BGM,
            BMP,
            NOTE,
            INVISIBLE,
            BOMB
        } type;
        float fraction;
        float time;
        union
        {
            struct
            {
                int key;
            } bgm;
            struct
            {
                int key;
                int layer;
            } bmp;
            struct
            {
                int player;
                int line;
                int key;
                bool end;
            } note;
            struct
            {
                int player;
                int line;
                int key;
            } misc;
        };
        bool executed;
        Note(float fraction, int key) : type(Type::BGM), fraction(fraction), time(0), executed(false), bgm({ key }) {}
        Note(float fraction, int key, int layer) : type(Type::BMP), fraction(fraction), time(0), executed(false), bmp({ key, layer }) {}
        Note(float fraction, int key, bool isInvisible, int player, int line) : type(isInvisible ? Type::INVISIBLE : Type::BOMB), fraction(fraction), time(0), executed(false), misc({ player, line, key }) {}
        Note(float fraction, int key, int player, int line, bool end) : type(Type::NOTE), fraction(fraction), time(0), executed(false), note({ player, line, key, end }) {}
    };

    struct Sector
    {
        float fraction;
        float time;
        float bpm;
        bool inclusive;
        Sector(float fraction, float time, float bpm, bool inclusive) : fraction(fraction), time(time), bpm(bpm), inclusive(inclusive) {}
    };

    struct Chart
    {
        int player;
        std::string genre;
        std::string title;
        std::string artist;
        std::string subtitle;
        std::string subartist;
        std::string stagefile;
        std::string banner;
        int playLevel;
        float total;
        int rank;
        std::string* wavs;
        std::string* bmps;
        float* signatures;
        std::vector<Note> notes;
        std::vector<Sector> sectors;

        Chart(const std::string& filename);
        ~Chart();

        float resolveSignatures(float fraction);

        float timeToFraction(float time);
    };

}

#endif