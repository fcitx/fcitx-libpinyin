#include <pinyin.h>
#include "pystring.h"

// Internal data definition

/**
* struct of pinyin token.
*
* this struct store the informations of a pinyin token
* (an initial or final)
*/
struct FcitxPinyinToken
{
    const char *latin; /**< Latin name of the token. */
    const char *zhuyin; /**< Zhuyin name in UTF-8. */
};


/**
* struct to index PinyinToken list.
*/
static const FcitxPinyinToken __pinyin_initials[] =
{
    {"", ""},
    {"b", "ㄅ"},
    {"c", "ㄘ"},
    {"ch","ㄔ"},
    {"d", "ㄉ"},
    {"f", "ㄈ"},
    {"h", "ㄏ"},
    {"g", "ㄍ"},
    {"k", "ㄎ"},
    {"j", "ㄐ"},
    {"m", "ㄇ"},
    {"n", "ㄋ"},
    {"l", "ㄌ"},
    {"r", "ㄖ"},
    {"p", "ㄆ"},
    {"q", "ㄑ"},
    {"s", "ㄙ"},
    {"sh","ㄕ"},
    {"t", "ㄊ"},
    {"w", "ㄨ"}, //Should be omitted in some case.
    {"x", "ㄒ"},
    {"y", "ㄧ"}, //Should be omitted in some case.
    {"z", "ㄗ"},
    {"zh","ㄓ"}
};

static const FcitxPinyinToken __pinyin_middles[] =
{
    {"", ""},
    {"i", "ㄧ"},
    {"u", "ㄨ"},
    {"v", "ㄩ"}
};

static const FcitxPinyinToken __pinyin_finals[] =
{
    {"", ""},
    {"a", "ㄚ"},
    {"ai", "ㄞ"},
    {"an", "ㄢ"},
    {"ang", "ㄤ"},
    {"ao", "ㄠ"},
    {"e", "ㄜ"},
    {"ea", "ㄝ"},
    {"ei", "ㄟ"},
    {"en", "ㄣ"},
    {"eng", "ㄥ"},
    {"er", "ㄦ"},
    {"ng", "ㄣ"},
    {"o", "ㄛ"},
    {"ong", "ㄨㄥ"},
    {"ou", "ㄡ"},
    {"in", "PINYIN_IN"},
    {"ing", "PINYIN_ING"}
};

static const char* __pinyin_middle_finals[4][18] =
{
    {"", "a",   "ai",  "an",  "ang",  "ao",  "e",  "ea",  "ei",  "en",   "eng",  "er",  "ng",  "o",   "ong",  "ou",  "in",  "ing" },
    {"i", "ia", "iai", "ian", "iang", "iao", "ie", "iea", "iei", "ien",  "ieng", "ier", "ing", "io",  "iong", "iu", "iin", "iing" },
    {"u", "ua", "uai", "uan", "uang", "uao", "ue", "uea", "uei", "un",   "ueng", "uer", "ung", "uo",  "uong", "uou", "uin", "uing" },
    {"v", "ua", "uai", "uan", "uang", "uao", "ve", "vea", "vei", "un",   "eng",  "ver", "vng", "uo",  "uong", "uou", "uin", "uing" },
};

static const FcitxPinyinToken __pinyin_tones [] =
{
    {"", ""},
    {"1", "ˉ"},
    {"2", "ˊ"},
    {"3", "ˇ"},
    {"4", "ˋ"},
    {"5", "˙"}
};

const char*
get_initial_string (ChewingKey* key)
{
    return __pinyin_initials [key->m_initial].latin;
}

const char*
get_middle_string (ChewingKey* key)
{
    guint16 middle = key->m_middle;
    return __pinyin_middles [middle].latin;
}

const char*
get_final_string (ChewingKey* key)
{
    guint16 final = key->m_final;
    return __pinyin_finals [final].latin;
}

const char*
get_middle_final_string (ChewingKey* key)
{
    const char* initial_string = get_initial_string(key);
    const char* middle_string = get_middle_string(key);
    guint16 middle = key->m_middle;
    guint16 final = key->m_final;

    /* wei */
    if (final != 0
        && ((initial_string[0] == 'y' && middle_string[0] == 'i')
            || (initial_string[0] == 'w' && middle_string[0] == 'u'))
    ) {
        middle = 0;
    }

    /* v -> u */
    if ((initial_string[0] == 'j' || initial_string[0] == 'q' || initial_string[0] == 'x' || initial_string[0] == 'y')
        && middle_string[0] == 'v') {
        middle = 2;
    }

    /* special hack */
    if (initial_string[0] == 'w' && middle == 0 && final == 14) {
        return "eng";
    }

    return __pinyin_middle_finals [middle][final];
}

const char*
get_tone_zhuyin_string (ChewingKey* key)
{
    return __pinyin_tones [key->m_tone].zhuyin;
}
