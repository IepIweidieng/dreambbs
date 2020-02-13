/*-------------------------------------------------------*/
/* xyz.c        ( NTHU CS MapleBBS Ver 3.10 )            */
/*-------------------------------------------------------*/
/* target : ���C���K���~��                               */
/* create : 09/04/08                                     */
/* author : cache                                        */
/* update :                                              */
/*-------------------------------------------------------*/


#include "bbs.h"
#include "bbs_script.h"

#include <netinet/tcp.h>
#include <sys/wait.h>
#include <sys/resource.h>

extern UCACHE *ushm;

/* cache.081017:�t�θ�T */

int
x_siteinfo(void)
{
    long nproc;
    double load[3], load_norm;
    nproc = sysconf(_SC_NPROCESSORS_ONLN);
    getloadavg(load, 3);
    load_norm = load[0] / ((nproc > 0) ? nproc : 2);

    clear();

    move(1, 0);
    prints("��    �W�G %s - %s\n", MYHOSTNAME, BBSIP);
    prints("�{�������G %s [%s]\x1b[m\n", BBSVERNAME, BBSVERSION);
    prints("���W�H�ơG %d/%d\n", ushm->count, MAXACTIVE);
    prints("�t�έt���G %.2f %.2f %.2f / %ld [%s]\n",
        load[0], load[1], load[2], nproc, load_norm > 5 ? "\x1b[1;41;37m�L��\x1b[m" : load_norm > 1 ?
        "\x1b[1;42;37m�y��\x1b[m" : "\x1b[1;44;37m���`\x1b[m");
    prints("���޸�ơG BRD %zu bytes, ACCT %zu bytes, HDR %zu bytes\n", sizeof(BRD), sizeof(ACCT), sizeof(HDR));
    prints("\n");
    prints("\x1b[1m�� BBS �����O�� WindTop BBS ���_�l�A\x1b[m\n");
    prints("\x1b[1m�ðѦҽѦ�e�������z�����睊�ӨӡA�Ҧ����z�]�����ݩ��@�̡C\x1b[m\n");
    prints("\n");
    prints("\x1b[1mDreamBBS.2010 Modified: Pang-Wei Tsai(cache)\x1b[m\n");
    prints("\x1b[1;33mInternet Technology Lab\x1b[37m, Institute of CCE, National Cheng Kung University.\x1b[m\n");
    prints("\n");
#ifdef Modules
    prints("\x1b[1;30mModules & Plug-in: \x1b[m\n\n");

//�Ҳդƪ���b�o��
#define ONLINE_STR(module)  "\x1b[1;32monline \x1b[1;30m " module "\x1b[m\n"
#define OFFLINE_STR(module) "\x1b[1;31moffline\x1b[1;30m " module "\x1b[m\n"
#define STATUS_STR(conf, module)  IF_ON(conf, ONLINE_STR, OFFLINE_STR)(module)

    prints(STATUS_STR(MultiRecommend, "Multi Recommend Control �h�ˤƱ��山��t��"));
    prints(STATUS_STR(M3_USE_PMORE, "pmore (piaip's more) 2007+ w/Movie"));
    prints(STATUS_STR(M3_USE_PFTERM, "pfterm (piaip's flat terminal, Perfect Term)"));
    prints(STATUS_STR(GRAYOUT, "Grayout Advanced Control �H�J�H�X�S�Ĩt��"));

#ifdef HAVE_BBSLUA
    prints(STATUS_STR(M3_USE_BBSLUA, "BBS-Lua " BBSLUA_VERSION_STR IF_ON(M3_USE_BBSLUA, " / " LUA_RELEASE IF_ON(BBSLUA_USE_LUAJIT, " / " LUAJIT_VERSION))));
#endif

//    prints(STATUS_STR(SMerge, "Smart Merge �פ�۰ʦX��));

#ifdef HAVE_BBSRUBY
    prints(STATUS_STR(M3_USE_BBSRUBY, "(EXP) BBSRuby " BBSRUBY_VERSION_STR " Interface: " BBSRUBY_INTERFACE_VER_STR IF_ON(M3_USE_BBSRUBY, " / Ruby " RUBY_RELEASE_STR IF_ON(BBSRUBY_USE_MRUBY, " / " MRUBY_RUBY_ENGINE " " MRUBY_VERSION))));
#endif

#else
//    prints("\x1b[1;30mModules & Plug-in: None\x1b[m\n");
#endif  /* #ifdef Modules */
    vmsg(NULL);
    return 0;
}
