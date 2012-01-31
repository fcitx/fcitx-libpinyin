#include <pinyin.h>

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
get_tone_zhuyin_string (ChewingKey* key)
{
    return __pinyin_tones [key->m_tone].zhuyin;
}
