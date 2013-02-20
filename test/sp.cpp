#include "pystring.h"
#include "config.h"
#include <string>
#include <iostream>
#include <assert.h>

bool test_sp(pinyin_instance_t* inst, const char* sp, const char* expect)
{
    pinyin_reset(inst);
    pinyin_parse_more_double_pinyins(inst, sp);
    std::string result;
    for (int i = 0; i < inst->m_pinyin_keys->len; i ++)
    {
        PinyinKey* pykey = &g_array_index(inst->m_pinyin_keys, PinyinKey, i);
        PinyinKeyPos* pykeypos = &g_array_index(inst->m_pinyin_key_rests, PinyinKeyPos, i);

        if (pykeypos->length() == 2) {
            const char* initial = 0;
            if (pykey->m_initial == CHEWING_ZERO_INITIAL)
                initial = "'";
            else
                initial = get_initial_string(pykey);
            result += initial;

            result += get_middle_final_string(pykey);
        }
        else if (pykeypos->length() == 1) {
            gchar* pystring = pykey->get_pinyin_string();

            result += pystring;
            g_free(pystring);
        }
        // std::cout << pykey->m_initial << " " << pykey->m_middle << " " << pykey->m_final << std::endl;
    }

    // std::cout << result << " " << expect;

    return (result == expect);
}

int main()
{
    pinyin_context_t* context = pinyin_init(LIBPINYIN_PKGDATADIR "/data", NULL);
    pinyin_set_double_pinyin_scheme(context, DOUBLE_PINYIN_ZRM);
    pinyin_instance_t* inst = pinyin_alloc_instance(context);

    pinyin_set_options(context, IS_PINYIN | USE_DIVIDED_TABLE | USE_RESPLIT_TABLE);

    assert(test_sp(inst, "xp", "xun"));
    assert(test_sp(inst, "xt", "xue"));
    assert(test_sp(inst, "xr", "xuan"));
    assert(test_sp(inst, "xu", "xu"));
    assert(test_sp(inst, "xs", "xiong"));
    assert(test_sp(inst, "xy", "xing"));
    assert(test_sp(inst, "xx", "xie"));
    assert(test_sp(inst, "xc", "xiao"));
    assert(test_sp(inst, "xd", "xiang"));
    assert(test_sp(inst, "xm", "xian"));
    assert(test_sp(inst, "xw", "xia"));
    assert(test_sp(inst, "xi", "xi"));

    assert(test_sp(inst, "nt", "nve"));
    assert(test_sp(inst, "nv", "nv"));
    assert(test_sp(inst, "no", "nuo"));
    assert(test_sp(inst, "nt", "nve"));
    assert(test_sp(inst, "nr", "nuan"));
    assert(test_sp(inst, "nu", "nu"));
    assert(test_sp(inst, "nb", "nou"));
    assert(test_sp(inst, "ns", "nong"));
    assert(test_sp(inst, "nq", "niu"));
    assert(test_sp(inst, "ny", "ning"));
    assert(test_sp(inst, "nn", "nin"));
    assert(test_sp(inst, "nx", "nie"));
    assert(test_sp(inst, "nc", "niao"));
    assert(test_sp(inst, "nd", "niang"));
    assert(test_sp(inst, "nm", "nian"));
    assert(test_sp(inst, "ni", "ni"));
    assert(test_sp(inst, "ng", "neng"));
    assert(test_sp(inst, "nf", "nen"));
    assert(test_sp(inst, "nz", "nei"));
    assert(test_sp(inst, "ne", "ne"));
    assert(test_sp(inst, "nk", "nao"));
    assert(test_sp(inst, "nh", "nang"));
    assert(test_sp(inst, "nj", "nan"));
    assert(test_sp(inst, "nl", "nai"));
    assert(test_sp(inst, "na", "na"));

    assert(test_sp(inst, "wo", "wo"));
    assert(test_sp(inst, "wu", "wu"));
    assert(test_sp(inst, "wg", "weng"));
    assert(test_sp(inst, "wf", "wen"));
    assert(test_sp(inst, "wz", "wei"));
    assert(test_sp(inst, "wh", "wang"));
    assert(test_sp(inst, "wj", "wan"));
    assert(test_sp(inst, "wl", "wai"));
    assert(test_sp(inst, "wa", "wa"));

    assert(test_sp(inst, "yp", "yun"));
    assert(test_sp(inst, "yt", "yue"));
    assert(test_sp(inst, "yv", "yu"));
    assert(test_sp(inst, "yo", "yo"));
    assert(test_sp(inst, "yr", "yuan"));
    assert(test_sp(inst, "yu", "yu"));
    assert(test_sp(inst, "yb", "you"));
    assert(test_sp(inst, "ys", "yong"));
    assert(test_sp(inst, "yy", "ying"));
    assert(test_sp(inst, "yn", "yin"));
    assert(test_sp(inst, "yi", "yi"));
    assert(test_sp(inst, "ye", "ye"));
    assert(test_sp(inst, "yk", "yao"));
    assert(test_sp(inst, "yh", "yang"));
    assert(test_sp(inst, "yj", "yan"));
    assert(test_sp(inst, "ya", "ya"));

    pinyin_free_instance(inst);
    pinyin_fini(context);

    return 0;
}
