#include <iostream>
#include <iconv.h>
#include <string>
#include <string.h>
#include <pinyin.h>
#include "config.h"

#define RET_BUF_LEN 256

using namespace std;

guint get_pinyin_cursor (pinyin_instance_t* inst, int cursor)
{
    /* Translate cursor position to pinyin position. */
    PinyinKeyPosVector & pinyin_poses = inst->m_pinyin_key_rests;
    guint pinyin_cursor = pinyin_poses->len;
    for (size_t i = 0; i < pinyin_poses->len; ++i) {
        PinyinKeyPos *pos = &g_array_index
            (pinyin_poses, PinyinKeyPos, i);
        if (pos->m_raw_begin <= cursor && cursor < pos->m_raw_end)
            pinyin_cursor = i;
    }

    return pinyin_cursor;
}

guint get_lookup_cursor (pinyin_instance_t* inst, int cursor)
{
    PinyinKeyVector & pinyins = inst->m_pinyin_keys;
    guint lookup_cursor = get_pinyin_cursor (inst, cursor);
    /* show candidates when pinyin cursor is at end. */
    if (lookup_cursor == pinyins->len)
        lookup_cursor = 0;
    return lookup_cursor;
}

int main(int argc, char *argv[])
{
    pinyin_context_t* context = pinyin_init(LIBPINYIN_PKGDATADIR "/data", NULL);
    pinyin_instance_t* inst = pinyin_alloc_instance(context);

    pinyin_set_options(context, IS_PINYIN | USE_DIVIDED_TABLE | USE_RESPLIT_TABLE);

    string s;
    cin >> s ;
    pinyin_parse_more_double_pinyins(inst, s.c_str());

    int cursor = 0;

    for (int i = 0; i < inst->m_pinyin_keys->len; i ++)
    {
        PinyinKey* pykey = &g_array_index(inst->m_pinyin_keys, PinyinKey, i);
        gchar* py = pykey->get_pinyin_string();
        gchar* chewing = pykey->get_chewing_string();
        cout << py << " "
             << chewing
             << endl;

        g_free(py);
        g_free(chewing);
    }

    while (true)
    {
        cout << get_lookup_cursor(inst, cursor) << endl;
        GArray* array = g_array_new(FALSE, FALSE, sizeof(lookup_candidate_t));
        pinyin_get_candidates(inst, get_lookup_cursor(inst, cursor), array);
        cout << array->len << endl;

        pinyin_guess_sentence(inst);

        char* sentence = NULL;
        pinyin_get_sentence(inst, &sentence);
        if (sentence)
            cout << sentence << endl;
        else
            cout << "no sentence" << endl;
        g_free(sentence);

        for (int i = 0 ; i < array->len; i ++ )
        {
            lookup_candidate_t token = g_array_index(array, lookup_candidate_t, i);
            char* word = NULL;
            pinyin_translate_token(inst, token.m_token, &word);
            if (word)
                cout << word << " ";
            g_free(word);
        }

        cout << "constraints " << inst->m_constraints->len << endl;

        int cand;
        cin >> cursor >> cand;

        if (cand >= 0)
            pinyin_choose_candidate(inst, 0, &g_array_index(array, lookup_candidate_t, cand));
        else if (cand != -1) {
            pinyin_clear_constraints(inst);
        }
        else if (cand != -2) {
            break;
        }

        g_array_free(array, TRUE);
    }
    pinyin_free_instance(inst);
    pinyin_fini(context);
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
