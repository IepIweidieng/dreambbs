/*-------------------------------------------------------*/
/* menu.c       ( NTHU CS MapleBBS Ver 3.00 )            */
/*-------------------------------------------------------*/
/* target : menu/help/movie routines                     */
/* create : 95/03/29                                     */
/* update : 2000/01/02                                   */
/*-------------------------------------------------------*/

#include "bbs.h"

#ifdef  HAVE_INFO
#define INFO_EMPTY      "Info      �i \x1b[1;36m�դ褽�i��\x1b[m �j"
#define INFO_HAVE       "Info      �i \x1b[41;33;1;5m�ֶi�Ӭݬ�\x1b[m �j"
#endif

#ifdef  HAVE_STUDENT
#define STUDENT_EMPTY   "1Student  �i \x1b[1;36m�ǥͤ��i��\x1b[m �j"
#define STUDENT_HAVE    "1Student  �i \x1b[41;33;1;5m�ֶi�Ӭݬ�\x1b[m �j"
#endif

static int
system_result(void)
{
    char fpath[128];
    sprintf(fpath, "brd/%s/@/@vote", brd_sysop);
    more(fpath, 0);
    return 0;
}


static int
view_login_log(void)
{
    char fpath[128];
    usr_fpath(fpath, cuser.userid, FN_LOG);
    more(fpath, 0);
    return 0;
}

static int
menumore(const void *arg)
{
    more((const char *)arg, 0);
    return 0;
}


static int
welcome(void)
{
    film_out(FILM_WELCOME, -1);
    return 0;
}

static int
resetbrd(void)
{
    board_main();
    return 0;
}

#if 0  // Unused
static int
x_sysload(void)
{
    long nproc;
    double load[3];
    char buf[80];

    /*load = ushm->sysload;*/
    nproc = sysconf(_SC_NPROCESSORS_ONLN);
    getloadavg(load, 3);
    sprintf(buf, "�t�έt�� %.2f %.2f %.2f / %ld", load[0], load[1], load[2], nproc);
    vmsg(buf);
    return XEASY;
}
#endif


/* ----------------------------------------------------- */
/* ���} BBS ��                                           */
/* ----------------------------------------------------- */


#define FN_NOTE_PAD     "run/note.pad"
#define FN_NOTE_TMP     "run/note.tmp"
#define NOTE_MAX        100
#define NOTE_DUE        48


typedef struct
{
    time_t tpad;
    char msg[356];
}      Pad;  /* DISKDATA(raw) */


int
pad_view(void)
{
    int fd, count;
    Pad *pad;

    if ((fd = open(FN_NOTE_PAD, O_RDONLY)) < 0)
        return XEASY;

    clear();
    move(0, 23);
    outs("\x1b[1;37;45m ��  " BOARDNAME " �d �� �O  �� \n\n");
    count = 0;

    mgets(-1);

    for (;;)
    {
        pad = (Pad *) mread(fd, sizeof(Pad));
        if (!pad)
        {
            vmsg(NULL);
            break;
        }

        outs(pad->msg);
        if (++count == 5)
        {
            move(b_lines, 0);
            outs("�Ы� [SPACE] �~���[��A�Ϋ���L�䵲���G");
            if (vkey() != ' ')
                break;

            count = 0;
            move(2, 0);
            clrtobot();
        }
    }

    close(fd);
    return 0;
}


static void
pad_draw(void)
{
    int i, cc, fdr, len;
    FILE *fpw;
    Pad pad;
    char *str, str2[300], buf[3][80];

    do
    {
        buf[0][0] = buf[1][0] = buf[2][0] = '\0';
        move(12, 0);
        clrtobot();
        outs("\n�Яd�� (�ܦh�T��)�A��[Enter]����");
        for (i = 0; (i < 3) &&
            vget(16 + i, 0, "�G", buf[i], 70, DOECHO); i++);
        cc = vans("(S)�s���[�� (E)���s�ӹL (Q)��F�H[S] ");
        if (cc == 'q' || i == 0)
            return;
    } while (cc == 'e');

    time(&(pad.tpad));

    str = pad.msg;

    sprintf(str, "\x1b[1;37;46m�W\x1b[34;47m %s \x1b[33m(%s)", cuser.userid, cuser.username);
    len = strlen(str);
    strcat(str, & " \x1b[30;46m"[len % 2U]);

    for (i = len / 2U; i < 41; i++)
        strcat(str, "�e");
    sprintf(str2, "\x1b[34;47m %.14s \x1b[37;46m�W\x1b[m\n%-70.70s\n%-70.70s\n%-70.70s\n",
        Etime(&(pad.tpad)), buf[0], buf[1], buf[2]);
    strcat(str, str2);

    f_cat(FN_NOTE_ALL, str);

    if (!(fpw = fopen(FN_NOTE_TMP, "w")))
        return;
    len = 342;  // strlen(str)
    memset(str + len + 1, 0, sizeof(pad.msg) - (len + 1));
    str[355] = '\n';

    fwrite(&pad, sizeof(pad), 1, fpw);

    if ((fdr = open(FN_NOTE_PAD, O_RDONLY)) >= 0)
    {
        Pad *pp;

        i = 0;
        cc = pad.tpad - NOTE_DUE * 60 * 60;
        mgets(-1);
        while ((pp = (Pad *) mread(fdr, sizeof(Pad))))
        {
            fwrite(pp, sizeof(Pad), 1, fpw);
            if ((++i > NOTE_MAX) || (pp->tpad < cc))
                break;
        }
        close(fdr);
    }

    fclose(fpw);

    rename(FN_NOTE_TMP, FN_NOTE_PAD);
    pad_view();
}


static int
goodbye(void)
{
    char ans;
    bmw_save();
    if (cuser.ufo2 & UFO2_DEF_LEAVE)
    {
        if (!(ans = vans("G)�A�O" NICKNAME " M)���i���� N)�d���O Q)�����H[Q] ")))
            ans = 'q';
    }
    else
        ans = vans("G)�A�O" NICKNAME " M)���i���� N)�d���O Q)�����H[Q] ");

    switch (ans)
    {
    case 'g':
    case 'y':
        break;

    case 'm':
        mail_sysop();
        break;

    case 'n':
        /* if (cuser.userlevel) */
        if (HAS_PERM(PERM_POST)) /* Thor.990118: �n��post�~��d��, �������e */
            pad_draw();
        break;

    case 'q':
        return XEASY;
    default:
        return XEASY; /* 090911.cache: ���p�߫������n�����H�a ;( */
    }

    return QUIT;
}


/* ----------------------------------------------------- */
/* help & menu processing                                */
/* ----------------------------------------------------- */

void
vs_mid(
    const char *mid)
{
    int spc, len, pad;
    unsigned int ufo;

    if (mid == NULL)
        mid = str_site;

    len = strlen(mid);
    ufo = cutmp->ufo;
    if (ufo & UFO_BIFF)
    {
        mid = NEWMAILMSG; // �A���s����
        len = 15;
    }
    else if (ufo & UFO_BIFFN)
    {
        mid = NEWPASSMSG; // �A���s�d��
        len = 15;
    }

    spc = b_cols - len; /* spc: �����ٳѤU�h�����Ŷ� */
    pad = spc / 2U; /* pad: Spaces needed to center `mid` */

    prints("%*s%s%*s\x1b[m\n", pad, "", mid, spc - pad, "");
}

void
vs_head(
    const char *title, const char *mid)
{
    int spc, len, len_ttl, pad;
    unsigned int ufo;
#ifdef  COLOR_HEADER
/*  int color = (time(0) % 7) + 41;        lkchu.981201: random color */
    int color = 44; //090911.cache: �Ӫ�F�T�w�@���C��
#endif

    if (mid == NULL)
    {
        move(0, 0);
        clrtoeol();
        mid = str_site;
    }
    else
    {
        clear();
    }

    len_ttl = strcspn(title, "\n");
    len = strlen(mid);
    ufo = cutmp->ufo;
    if (ufo & UFO_BIFF)
    {
        mid = NEWMAILMSG; // �A���s����
        len = 15;
    }
    else if (ufo & UFO_BIFFN)
    {
        mid = NEWPASSMSG; // �A���s�d��
        len = 15;
    }

    spc = b_cols - 14 - len - strlen(currboard); /* spc: �����ٳѤU�h�����Ŷ� */
    len_ttl = BMIN(len_ttl, spc); /* Truncate `title` if too long */
    spc -= len_ttl; /* �\�� title �H��A�����٦� spc ��Ŷ� */
    pad = BMAX((b_cols - len)/2U - (len_ttl + 5), 0U); /* pad: Spaces needed to center `mid` */

#ifdef  COLOR_HEADER
    prints("\x1b[1;%2d;37m�i%.*s�j%*s \x1b[33m%s\x1b[1;%2d;37m%*s \x1b[37m�ݪO�m%s�n\x1b[m\n",
        color, len_ttl, title, pad, "", mid, color, spc - pad, "", currboard);
#else
    prints("\x1b[1;46;37m�i%.*s�j%*s \x1b[33m%s\x1b[46m%*s \x1b[37m�ݪO�m%s�n\x1b[m\n",
        len_ttl, title, pad, "", mid, spc - pad, "", currboard);
#endif
}


void clear_notification(void)
{
    unsigned int ufo = cutmp->ufo;

    if (ufo & UFO_BIFF)
        cutmp->ufo = ufo ^ UFO_BIFF;     /* �ݹL�N�� */
    if (ufo & UFO_BIFFN)
        cutmp->ufo = ufo ^ UFO_BIFFN;     /* �ݹL�N�� */
}


/* ------------------------------------- */
/* �ʵe�B�z                              */
/* ------------------------------------- */


static char footer[160];


void
movie(void)
{
    if (cuser.ufo2 & UFO2_MOVIE)
    {
        static int tag = FILM_MOVIE;

        tag = film_out(tag, 2);
    }
}

static void
menu_foot(void)
{
    static int orig_flag = -1;
    static time_t uptime = -1;
    static char flagmsg[16];
    static char datemsg[16];

    int ufo;
    time_t now;

    ufo = cuser.ufo;
    time(&now);

    /* Thor: �P�� ��� �I�s�� �n�ͤW�� ���� */

    ufo &= UFO_PAGER | UFO_CLOAK | UFO_QUIET | UFO_PAGER1 | UFO_MESSAGE;
    if (orig_flag != ufo)
    {
        orig_flag = ufo;
        sprintf(flagmsg,
            "%s/%s",
            (ufo & UFO_PAGER1) ? "����" : (ufo & UFO_PAGER) ? "�b��" : "���}",
            (ufo & UFO_MESSAGE) ? "����" : (ufo & UFO_QUIET) ? "�b��" : "���}");
    }

    if (now > uptime)
    {
        struct tm *ptime;

        ptime = localtime(&now);
        sprintf(datemsg, "[%d/%d �P��%.2s ",
            ptime->tm_mon + 1, ptime->tm_mday,
            & "�Ѥ@�G�T�|����"[2 * ptime->tm_wday]);

        uptime = now + 86400 - ptime->tm_hour * 3600 -
            ptime->tm_min * 60 - ptime->tm_sec;
    }
    ufo = (now - (uptime - 86400)) / 60;

    /* Thor.980913: ����: �̱`���I�s movie()���ɾ��O�C����s film, �b 60��H�W,
                          �G���ݰw�� xx:yy �ӯS�O�@�@�r���x�s�H�[�t */

    sprintf(footer, "\x1b[0;34;46m%s%d:%02d] \x1b[30;47m �ثe���W��\x1b[31m%4d\x1b[30m �H�A�ڬO \x1b[31m%-*s\t\x1b[30m [�I�s/�T��]\x1b[31m%s",
        datemsg, ufo / 60, ufo % 60,
        /*ushm->count*/total_num,
        12 + 14 - strlen(datemsg) + (ufo / 60 < 10),
        cuser.userid, flagmsg);
    outf(footer);
}


#define MENU_CHANG      0x80000000


#define PERM_MENU       PERM_PURGE


#ifdef __cplusplus
  #define INTERNAL extern  /* Used inside an anonymous namespace */
  #define INTERNAL_INIT /* Empty */
#else
  #define INTERNAL static
  #define INTERNAL_INIT static
#endif

#ifdef __cplusplus
namespace {
#endif

INTERNAL MENU menu_main[];
INTERNAL MENU menu_service[];
INTERNAL MENU menu_xyz[];
INTERNAL MENU menu_user[];
INTERNAL MENU menu_song[];


/* ----------------------------------------------------- */
/* load menu                                             */
/* ----------------------------------------------------- */
INTERNAL MENU menu_admin[];

INTERNAL_INIT MENU menu_boardadm[] =
{
    {{m_newbrd}, PERM_BOARD, M_SYSTEM,
    "NewBoard   �}�P�s�ݪO"},

    {{a_editbrd}, PERM_BOARD, M_SYSTEM,
    "ConfigBrd  �]�w�ݪO"},

    {{m_bmset}, PERM_BOARD, M_SYSTEM,
    "BMset      �]�w���D�v��"},

    {{BanMail}, PERM_BOARD|PERM_SYSOP, M_BANMAIL,
    "FireWall   �׫H�C��"},

    {{.dlfunc = DL_NAME("adminutil.so", bm_check)}, PERM_BOARD|PERM_SYSOP, M_DL(M_XMODE),
    "Manage     �O�D�T�{"},

    {{.dlfunc = DL_NAME("adminutil.so", m_expire)}, PERM_BOARD|PERM_SYSOP, M_DL(M_XMODE),
    "DExpire    �M���ݪO�R���峹"},

    {{.dlfunc = DL_NAME("adminutil.so", mail_to_bm)}, PERM_SYSOP, M_DL(M_XMODE),
    "ToBM       �H�H���O�D"},

    {{.dlfunc = DL_NAME("adminutil.so", mail_to_all)}, PERM_SYSOP, M_DL(M_XMODE),
    "Alluser    �t�γq�i"},

    {{.dlfunc = DL_NAME("personal.so", personal_admin)}, PERM_BOARD|PERM_SYSOP, M_DL(M_XMODE),
    "Personal   �ӤH�O�f��"},

    {{.menu = menu_admin}, PERM_MENU + 'N', M_ADMIN,
    "�ݪO�`��"}
};

INTERNAL_INIT MENU menu_accadm[] =
{
    {{m_user}, PERM_ACCOUNTS, M_SYSTEM,
    "User       �ϥΪ̸��"},

    {{.dlfunc = DL_NAME("bank.so", money_back)}, PERM_ACCOUNTS, M_DL(M_XMODE),
    "GetMoney   �פJ�¹ڹ�"},

#ifdef  HAVE_SONG
    {{.dlfunc = DL_NAME("song.so", AddRequestTimes)}, PERM_KTV, M_DL(M_XMODE),
    "Addsongs   �W�[�I�q����"},
#endif

    {{.dlfunc = DL_NAME("passwd.so", new_passwd)}, PERM_SYSOP, M_DL(M_XMODE),
    "Password   ���e�s�K�X"},

#ifdef  HAVE_REGISTER_FORM
    {{.dlfunc = m_register}, PERM_ACCOUNTS, M_SYSTEM,
    "1Register  �f�ֵ��U���"},
#endif

#ifdef HAVE_OBSERVE_LIST
    {{.dlfunc = DL_NAME("observe.so", Observe_list)}, PERM_SYSOP|PERM_BOARD, M_DL(M_XMODE),
    "2Observe   �t���[��W��"},
#endif

    {{.menu = menu_admin}, PERM_MENU + 'U', M_ADMIN,
    "���U�`��"}
};

INTERNAL_INIT MENU menu_settingadm[] =
{

    {{.dlfunc = DL_NAME("adminutil.so", m_xfile)}, PERM_SYSOP, M_DL(M_XFILES),
    "File       �s��t���ɮ�"},

    {{.dlfunc = DL_NAME("adminutil.so", m_xhlp)}, PERM_SYSOP, M_DL(M_XFILES),
    "Hlp        �s�軡���ɮ�"},

    {{.dlfunc = DL_NAME("admin.so", Admin)}, PERM_SYSOP, M_DL(M_XMODE),
    "Operator   �t�κ޲z���C��"},

    {{.dlfunc = DL_NAME("chatmenu.so", Chatmenu)}, PERM_CHATROOM|PERM_SYSOP, M_DL(M_XMODE),
    "Chatmenu   " CHATROOMNAME "�ʵ�"},

    {{.dlfunc = DL_NAME("violate.so", Violate)}, PERM_SYSOP, M_DL(M_XMODE),
    "Violate    �B�@�W��"},

    {{.dlfunc = DL_NAME("adminutil.so", special_search)}, PERM_SYSOP, M_DL(M_XMODE),
    "XSpecial   �S��j�M"},

    {{.dlfunc = DL_NAME("adminutil.so", update_all)}, PERM_SYSOP, M_DL(M_XMODE),
    "Database   �t�θ�Ʈw��s"},

    {{.menu = menu_admin}, PERM_MENU + 'X', M_ADMIN,
    "�t�θ��"}
};

/* ----------------------------------------------------- */
/* reset menu                                            */
/* ----------------------------------------------------- */
INTERNAL_INIT MENU menu_reset[] =
{
    {{.dlfuncarg = {DL_NAME("adminutil.so", m_resetsys), (const void *)1}}, PERM_BOARD, M_DL(M_XMODE | M_ARG),
    "Camera     �ʺA�ݪO"},

    {{.dlfuncarg = {DL_NAME("adminutil.so", m_resetsys), (const void *)2}}, PERM_BOARD, M_DL(M_XMODE | M_ARG),
    "Group      �����s��"},

    {{.dlfuncarg = {DL_NAME("adminutil.so", m_resetsys), (const void *)3}}, PERM_SYSOP, M_DL(M_XMODE | M_ARG),
    "Mail       �H�H���H��H"},

    {{.dlfuncarg = {DL_NAME("adminutil.so", m_resetsys), (const void *)4}}, PERM_ADMIN, M_DL(M_XMODE | M_ARG),
    "Killbbs    �M�������` BBS"},

    {{.dlfuncarg = {DL_NAME("adminutil.so", m_resetsys), (const void *)5}}, PERM_BOARD, M_DL(M_XMODE | M_ARG),
    "Firewall   �׫H�C��"},

    {{.dlfuncarg = {DL_NAME("adminutil.so", m_resetsys), (const void *)6}}, PERM_CHATROOM, M_DL(M_XMODE | M_ARG),
    "Xchatd     ���}��ѫ�"},

    {{.dlfuncarg = {DL_NAME("adminutil.so", m_resetsys), (const void *)7}}, PERM_SYSOP, M_DL(M_XMODE | M_ARG),
    "All        ����"},

    {{.menu = menu_admin}, PERM_MENU + 'K', M_ADMIN,
    "�t�έ��m"}
};


/* ----------------------------------------------------- */
/* administrator's maintain menu                         */
/* ----------------------------------------------------- */


INTERNAL_INIT MENU menu_admin[] =
{

    {{.menu = menu_accadm}, PERM_ADMIN, M_ADMIN,
    "Acctadm    ���U�`�ޥ\\��"},

    {{.menu = menu_boardadm}, PERM_BOARD, M_ADMIN,
    "Boardadm   �ݪO�`�ޥ\\��"},

    {{.menu = menu_settingadm}, PERM_ADMIN, M_ADMIN,
    "Data       �t�θ�Ʈw�]�w"},

    {{.dlfunc = DL_NAME("innbbs.so", a_innbbs)}, PERM_BOARD, M_DL(M_SYSTEM),
    "InnBBS     ��H�]�w"},

    {{.menu = menu_reset}, PERM_ADMIN, M_ADMIN,
    "ResetSys   ���m�t��"},

#ifdef  HAVE_ADM_SHELL
    {{x_csh}, PERM_SYSOP, M_SYSTEM,
    "Shell      ����t�� Shell"},
#endif

#ifdef  HAVE_REPORT
    {{m_trace}, PERM_SYSOP, M_SYSTEM,
    "Trace      �]�w�O�_�O��������T"},
#endif

    {{.menu = menu_main}, PERM_MENU + 'A', M_ADMIN,
    "�t�κ��@"}
};

#ifdef __cplusplus
}  // namespace
#endif


/* ----------------------------------------------------- */
/* mail menu                                             */
/* ----------------------------------------------------- */


static int
XoMbox(void)
{
    if (HAS_PERM(PERM_DENYMAIL))
        vmsg("�z���H�c�Q��F�I");
    else
        xover(XZ_MBOX);
    return 0;
}

#ifdef HAVE_SIGNED_MAIL
int m_verify(void);
#endif
int m_setmboxdir(void);

#ifdef __cplusplus
namespace {
#endif
INTERNAL_INIT MENU menu_mail[] =
{
    {{XoMbox}, PERM_READMAIL, M_RMAIL,
    "Read       �\\Ū�H��"},

    {{m_send}, PERM_INTERNET, M_SMAIL,
    "MailSend   �����H�H"},

#ifdef MULTI_MAIL  /* Thor.981009: ����R�����B�H */
    {{mail_list}, PERM_INTERNET, M_SMAIL,
    "Group      �s�ձH�H"},
#endif

    {{.dlfunc = DL_NAME("contact.so", Contact)}, PERM_INTERNET, M_DL(M_XMODE),
    "Contact    �p���W��"},

    {{m_setforward}, PERM_INTERNET, M_SMAIL,
    "AutoFor    �����H�۰���H"},

    {{m_setmboxdir}, PERM_INTERNET, M_SMAIL,
    "Fixdir     ���ثH�c����"},

#ifdef HAVE_DOWNLOAD
    {{m_zip}, PERM_VALID, M_XMODE,
    "Zip        ���]�U���ӤH���"},
#endif
/*
#ifdef HAVE_SIGNED_MAIL
    {{m_verify}, PERM_VALID, M_XMODE,
    "Verify     ���ҫH��q�lñ��"},
#endif
*/
    {{mail_sysop}, PERM_BASIC, M_SMAIL,
    "Yes Sir!   �H�H������"},

    {{.menu = menu_main}, PERM_MENU + 'R', M_MMENU,       /* itoc.020829: �� guest �S�ﶵ */
    "�q�l�l��"}
};
#ifdef __cplusplus
}  // namespace
#endif


/* ----------------------------------------------------- */
/* Talk menu                                             */
/* ----------------------------------------------------- */

static int
XoUlist(void)
{
    xover(XZ_ULIST);
    return 0;
}


#ifdef __cplusplus
namespace {
#endif

INTERNAL_INIT MENU menu_talk[] =
{
    {{XoUlist}, 0, M_LUSERS,
    "Users      ������Ѥ�U"},

    {{t_pal}, PERM_BASIC, M_PAL,
    "Friend     �s��n�ͦW��"},

#ifdef  HAVE_BANMSG
    {{t_banmsg}, PERM_BASIC, M_XMODE,
    "Banmsg     �ڦ��T���W��"},
#endif
    {{.dlfunc = DL_NAME("aloha.so", t_aloha)}, PERM_BASIC, M_DL(M_XMODE),
    "Aloha      �W���q���W��"},

    {{t_pager}, PERM_BASIC, M_XMODE,
    "Pager      �����I�s��"},

    {{t_message}, PERM_BASIC, M_XMODE,
    "Message    �����T��"},

    {{t_query}, 0, M_QUERY,
    "Query      �d�ߺ���"},

    /* Thor.990220: chatroom client ��ĥ~�� */
    {{.dlfunc = DL_NAME("chat.so", t_chat)}, PERM_CHAT, M_DL(M_CHAT),
    "ChatRoom   " NICKNAME CHATROOMNAME},

    {{t_recall}, PERM_BASIC, M_XMODE,
    "Write      �^�U�e�X�����T"},

#ifdef LOGIN_NOTIFY
    {{t_loginNotify}, PERM_PAGE, M_XMODE,
    "Notify     �]�w�t�κ��ͨ�M"},
#endif
    {{.menu = menu_main}, PERM_MENU + 'U', M_UMENU,
    "�𶢲��"}
};


/* ----------------------------------------------------- */
/* System menu                                           */
/* ----------------------------------------------------- */

INTERNAL_INIT MENU menu_information[] =
{

    {{.funcarg = {menumore, "gem/@/@pop"}}, 0, M_READA | M_ARG,
    "Login      �W�����ƱƦ�]"},

    {{.funcarg = {menumore, "gem/@/@-act"}}, 0, M_READA | M_ARG,
    "Today      ����W�u�H���έp"},

    {{.funcarg = {menumore, "gem/@/@=act"}}, 0, M_READA | M_ARG,
    "Yesterday  �Q��W�u�H���έp"},

    {{.funcarg = {menumore, "gem/@/@-day"}}, 0, M_READA | M_ARG,
    "0Day       ����Q�j�������D"},

    {{.funcarg = {menumore, "gem/@/@-week"}}, 0, M_READA | M_ARG,
    "1Week      ���g���Q�j�������D"},

    {{.funcarg = {menumore, "gem/@/@-month"}}, 0, M_READA | M_ARG,
    "2Month     ����ʤj�������D"},

    {{.funcarg = {menumore, "gem/@/@-year"}}, 0, M_READA | M_ARG,
    "3Year      ���~�צʤj�������D"},

    {{.menu = menu_xyz}, PERM_MENU + 'L', M_MMENU,
    "�έp���"}
};


INTERNAL_INIT MENU menu_xyz[] =
{
    {{.menu = menu_information}, 0, M_XMENU,
    "Tops       " NICKNAME "�Ʀ�]"},

    {{.funcarg = {menumore, FN_ETC_VERSION}}, 0, M_READA | M_ARG,
    "Version    ���X�o�i��T"},

    {{.dlfunc = DL_NAME("xyz.so", x_siteinfo)}, 0, M_DL(M_READA),
    "Xinfo      �t�ε{����T"},

    {{pad_view}, 0, M_READA,
    "Note       �[�ݤ߱��d���O"},

    {{welcome}, 0, M_READA,
    "Welcome    �[���w��e��"},

    {{.funcarg = {menumore, FN_ETC_COUNTER}}, 0, M_READA | M_ARG,
    "History    �������v�y��"},

    {{.menu = menu_main}, PERM_MENU + 'T', M_SMENU,
    "�t�θ�T"}
};

/* ----------------------------------------------------- */
/* User menu                                             */
/* ----------------------------------------------------- */

INTERNAL_INIT MENU menu_reg[] =
{

    {{u_info}, PERM_BASIC, M_XMODE,
    "Info       �]�w�ӤH��ƻP�K�X"},

    {{u_addr}, PERM_BASIC, M_XMODE,
    "Address    ��g�q�l�H�c�λ{��"},

    {{u_verify}, PERM_BASIC, M_UFILES,
    "Verify     ��g�m���U�{�ҽX�n"},

#ifdef  HAVE_REGISTER_FORM
    {{u_register}, PERM_BASIC, M_UFILES,
    "Register   ��g�m���U�ӽг�n"},
#endif

    {{u_setup}, PERM_VALID, M_UFILES,
    "Mode       �]�w�ާ@�Ҧ�"},

    {{ue_setup}, 0, M_UFILES,
    "Favorite   �ӤH�ߦn�]�w"},

    {{u_xfile}, PERM_BASIC, M_UFILES,
    "Xfile      �s��ӤH�ɮ�"},

    {{.dlfunc = DL_NAME("list.so", List)}, PERM_VALID, M_DL(M_XMODE),
    "1List      �s�զW��"},

    {{.menu = menu_user}, PERM_MENU + 'I', M_MMENU,
    "���U��T"}
};


INTERNAL_INIT MENU menu_user[] =
{
    {{.menu = menu_reg}, 0, M_XMENU,
    "Configure  ���U�γ]�w�ӤH��T"},

    {{u_lock}, PERM_BASIC, M_XMODE,
    "Lock       ��w�ù�"},

    {{.dlfunc = DL_NAME("memorandum.so", Memorandum)}, PERM_VALID, M_DL(M_XMODE),
    "Note       �Ƨѿ�"},

    {{.dlfunc = DL_NAME("pnote.so", main_note)}, PERM_VALID, M_DL(M_XMODE),
    "PNote      �ӤH������"},

#ifdef  HAVE_CLASSTABLEALERT
    {{.dlfunc = DL_NAME("classtable2.so", main_classtable)}, PERM_VALID, M_DL(M_XMODE),
    "2Table     �s���ӤH�\\�Ҫ�"},
#endif

    {{view_login_log}, PERM_BASIC, M_READA,
    "ViewLog    �˵��W������"},

    {{.menu = menu_service}, PERM_MENU + 'C', M_UMENU,
    "�ӤH�]�w"}
};


/* ----------------------------------------------------- */
/* tool menu                                             */
/* ----------------------------------------------------- */

INTERNAL MENU menu_service[];

/* ----------------------------------------------------- */
/* game menu                                             */
/* ----------------------------------------------------- */

INTERNAL_INIT MENU menu_game[] =
{
    {{.dlfunc = DL_NAME("bj.so", BlackJack)}, PERM_VALID, M_DL(M_XMODE),
    "BlackJack  " NICKNAME "�³ǧJ"},

    {{.dlfunc = DL_NAME("guessnum.so", fightNum)}, PERM_VALID, M_DL(M_XMODE),
    "FightNum   �Ʀr�j�M��"},

    {{.dlfunc = DL_NAME("guessnum.so", guessNum)}, PERM_VALID, M_DL(M_XMODE),
    "GuessNum   �̥ʲq�Ʀr"},

    {{.dlfunc = DL_NAME("mine.so", Mine)}, PERM_VALID, M_DL(M_XMODE),
    "Mine       " NICKNAME "��a�p"},

    {{.dlfunc = DL_NAME("pip.so", p_pipple)}, PERM_VALID, M_DL(M_XMODE),
    "Pip        " NICKNAME "�԰���"},

    {{.menu = menu_service}, PERM_MENU + 'F', M_UMENU,
    "�C����"}

};

/* ----------------------------------------------------- */
/* yzu menu                                              */
/* ----------------------------------------------------- */

INTERNAL_INIT MENU menu_special[] =
{

    {{.dlfunc = DL_NAME("personal.so", personal_apply)}, PERM_VALID, M_DL(M_XMODE),
    "PBApply      �ӽЭӤH�ݪO"},

    {{.dlfunc = DL_NAME("bank.so", bank_main)}, PERM_VALID, M_DL(M_XMODE),
    "Bank       �@�Ȧ�"},

    {{.dlfunc = DL_NAME("shop.so", shop_main)}, PERM_VALID, M_DL(M_XMODE),
    "Pay        �@�ө�"},

#ifdef HAVE_SONG
    {{.menu = menu_song}, PERM_VALID, M_XMENU,
    "Request      �I�q�t��"},
#endif

    {{resetbrd}, PERM_ADMIN, M_XMODE,
    "CameraReset  �������]"},

    {{.menu = menu_service}, PERM_MENU + 'R', M_UMENU,
    "�[�ȪA��"}
};



/* ----------------------------------------------------- */
/* song menu                                             */
/* ----------------------------------------------------- */

#ifdef HAVE_SONG
INTERNAL_INIT MENU menu_song[] =
{
    {{.dlfunc = DL_NAME("song.so", XoSongMain)}, PERM_VALID, M_DL(M_XMODE),
    "Request       �I�q�q��"},

    {{.dlfunc = DL_NAME("song.so", XoSongLog)}, PERM_VALID, M_DL(M_XMODE),
    "OrderSongs    �I�q����"},

    {{.dlfunc = DL_NAME("song.so", XoSongSub)}, PERM_VALID, M_DL(M_XMODE),
    "Submit        ��Z�M��"},

    {{.menu = menu_special}, PERM_MENU + 'R', M_XMENU,
    "���I�I�q"}
};
#endif


/* ----------------------------------------------------- */
/* service menu                                          */
/* ----------------------------------------------------- */

/* Thor.990224: �}��~���ɭ� */
INTERNAL_INIT MENU menu_service[] =
{

    {{.menu = menu_user}, 0, M_UMENU,
    "User      �i �ӤH�u��� �j"},

    {{.menu = menu_special}, PERM_VALID, M_XMENU,
    "Bonus     �i �[�ȪA�Ȱ� �j"},

    {{.menu = menu_game}, PERM_VALID, M_XMENU,
    "Game      �i �C������� �j"},

#ifdef  HAVE_INFO
    {{Information}, 0, M_BOARD,
    INFO_EMPTY},
#endif

#ifdef  HAVE_STUDENT
    {{Student}, 0, M_BOARD,
    STUDENT_EMPTY},
#endif

/* 091007.cache: �ԤH�鲼�S�N�q... */

    {{.dlfunc = DL_NAME("newboard.so", XoNewBoard)}, PERM_VALID, M_DL(M_XMODE),
    "Cosign    �i �s�p�ӽа� �j"},

    {{.dlfunc = DL_NAME("vote.so", SystemVote)}, PERM_POST, M_DL(M_XMODE),
    "Vote      �i �t�Χ벼�� �j"},

    {{system_result}, 0, M_READA,
    "Result    �i�t�Χ벼���G�j"},

/*
#ifdef HAVE_SONG
    {{.menu = menu_song}, PERM_VALID, M_XMENU,
    "Song      �i  �I�q�t�ΰ�  �j"},
#endif
*/
    {{.menu = menu_main}, PERM_MENU + 'U', M_UMENU,
     NICKNAME "�A��"}
};

#ifdef __cplusplus
}  // namespace
#endif

/* ----------------------------------------------------- */
/* main menu                                             */
/* ----------------------------------------------------- */

#ifdef  HAVE_CHANGE_SKIN
static int
sk_windtop_init(void)
{
    s_menu = menu_windtop;
    vmsg("DEBUG:DreamBBS");
    return SKIN;
}

#ifdef __cplusplus
namespace {
#endif
INTERNAL_INIT MENU skin_main[] =
{
    {{sk_windtop_init}, PERM_SYSOP, M_XMODE,
    "DreamBBS   �w�]���t��"},

    {{.menu = menu_main}, PERM_MENU + 'W', M_MMENU,
    "�������"}
};
#ifdef __cplusplus
}  // namespace
#endif
#endif  /* #ifdef  HAVE_CHANGE_SKIN */

static int
Gem(void)
{
    XoGem("gem/.DIR", "", ((HAS_PERM(PERM_SYSOP|PERM_BOARD|PERM_GEM)) ? GEM_SYSOP : GEM_USER));
    return 0;
}

#ifdef __cplusplus
namespace {
#endif

INTERNAL_INIT MENU menu_main[] =
{
    {{.menu = menu_admin}, PERM_ADMIN, M_ADMIN,
    "0Admin    �i �t�κ��@�� �j"},

    {{Gem}, 0, M_GEM,
    "Announce  �i ��ؤ��G�� �j"},

    {{Boards}, 0, M_BOARD,
    "Boards    �i \x1b[1;33m�G�i�Q�װ�\x1b[m �j"},

    {{Class}, 0, M_CLASS,
    "Class     �i \x1b[1;33m���հQ�װ�\x1b[m �j"},

#ifdef  HAVE_PROFESS
    {{Profess}, 0, M_PROFESS,
    "Profession�i \x1b[1;33m�M�~�Q�װ�\x1b[m �j"},
#endif

#ifdef  HAVE_FAVORITE
    {{MyFavorite}, PERM_BASIC, M_CLASS,
    "Favorite  �i \x1b[1;32m�ڪ��̷R��\x1b[m �j"},
#endif

#ifdef HAVE_SIGNED_MAIL
    {{.menu = menu_mail}, PERM_BASIC, M_MAIL, /* Thor.990413: �Y����m_verify, guest�N�S����椺�e�o */
    "Mail      �i �H��u��� �j"},
#else
    {{.menu = menu_mail}, PERM_BASIC, M_MAIL,
    "Mail      �i �p�H�H��� �j"},
#endif

    {{.menu = menu_talk}, 0, M_TMENU,
    "Talk      �i �𶢲�Ѱ� �j"},

    {{.menu = menu_service}, PERM_BASIC, M_XMENU,
    "DService  �i " NICKNAME "�A�Ȱ� �j"},

    /* lkchu.990428: ���n����b�ӤH�u��� */
    {{.menu = menu_xyz}, 0, M_SMENU,
    "Xyz       �i �t�θ�T�� �j"},

#ifdef  HAVE_CHANGE_SKIN
    {{.menu = *skin_main}, PERM_SYSOP, M_XMENU,
    "2Skin     �i ��ܤ����� �j"},
#endif

    {{goodbye}, 0, M_XMODE,
    "Goodbye   �i�A�O" BOARDNAME "�j"},

    {{NULL}, PERM_MENU + 'B', M_MMENU,
    "�D�\\���"}
};

#ifdef __cplusplus
}  // namespace
#endif

#ifdef  TREAT
static int
goodbye1(void)
{
    switch (vans("G)�A�O" NICKNAME " Q)�����H[Q] "))
    {
    case 'g':
    case 'y':
        return QUIT:
        break;

    case 'q':
    default:
        break;
    }

    clear();
    outs("�� ������z���q���չϧ������A�� ��\n");
    bell();
    vkey();
    outs("�� ����  �F�A����  ^O^ �A" BOARDNAME "���z�M�H�`�ּ� ��\n");
    bell();
    vkey();
    return QUIT:
}


#ifdef __cplusplus
namespace {
#endif
INTERNAL_INIT MENU menu_treat[] =
{
    {{goodbye1}, 0, M_XMODE,
    "Goodbye   �i�A�O" NICKNAME "�j"},

    {{NULL}, PERM_MENU + 'G', M_MMENU,
    "�D�\\���"}
};
#ifdef __cplusplus
}  // namespace
#endif
#endif  /* #ifdef  TREAT */

GCC_PURE
int strip_ansi_len(
    const char *str)
{
    int len;
    const char *ptr, *tmp;
    ptr = str;
    len = strlen(str);

    while (ptr)
    {
        ptr = strstr(ptr, "\x1b");
        if (ptr)
        {
            for (tmp=ptr; *tmp!='m'; tmp++);
            len -= (tmp-ptr+1);
            ptr = tmp+1;
        }
    }
    return len;
}


const char *
check_info(const char *input)
{
#if defined(HAVE_INFO) || defined(HAVE_STUDENT)
    const BRD *brd;
#endif
    const char *name = NULL;
    name = input;
#ifdef  HAVE_INFO
    if (!strcmp(input, INFO_EMPTY))
    {
        brd = bshm->bcache + brd_bno(BRD_BULLETIN);
        if (brd)
        {
            check_new(brd);
            if (brd->blast > brd_visit[brd_bno(BRD_BULLETIN)])
                name = INFO_HAVE;
        }
    }
#endif
#ifdef  HAVE_STUDENT
    if (!strcmp(input, STUDENT_EMPTY))
    {
        brd = bshm->bcache + brd_bno(BRD_SBULLETIN);
        if (brd)
        {
            check_new(brd);
            if (brd->blast > brd_visit[brd_bno(BRD_SBULLETIN)])
                name = STUDENT_HAVE;
        }
    }
#endif

    return name;
}


void
main_menu(void)
{
#ifdef  TREAT
    domenu(menu_treat, MENU_YPOS_REF, MENU_XPOS_REF, 0, 0, 1);
#endif
    domenu(menu_main, MENU_YPOS_REF, MENU_XPOS_REF, 0, 0, 1);
}

void
domenu(
    MENU *menu, int y_ref, int x_ref, int height_ref, int width_ref, int cmdcur_max)
{
    int y, x, height, width;
    MENU *mtail, *table[41];
    int cc=0, cx=0;     /* current / previous cursor position */
    int max=0, mmx=0;   /* current / previous menu max */
    int cmd=0;
    unsigned int mode = MENU_LOAD | MENU_DRAW | MENU_FILM;

    /* IID.20200107: Match input sequence. */
    int cmdcur[COUNTOF(table)];  /* Command matching cursor */
    int cmdlen[COUNTOF(table)];  /* Command matching length (no spaces) */
    bool keep_cmd = false;
    bool keyboard_cmd = true;
    int item_length[COUNTOF(table)]={0};
    int max_item_length = 0;

    y = gety_ref(y_ref);
    x = getx_ref(x_ref);
    height = gety_ref(height_ref);
    width = getx_ref(width_ref);

    for (;;)
    {
        if (!menu)
        {
            return;
        }
        if (!keep_cmd)
        {
            memset(cmdcur, 0, sizeof(cmdcur));
            memset(cmdlen, 0, sizeof(cmdlen));
        }
        keep_cmd = false;

        if (mode & MENU_LOAD)
        {
            unsigned int level = cuser.userlevel;
            unsigned int mlevel;
            max = -1;
            for (mtail = menu;; mtail++)
            {
                mlevel = mtail->level;
                if (mlevel & PERM_MENU)
                {

#ifdef  MENU_VERBOSE
                    if (max < 0)                /* �䤣��A�X�v�����\��A�^�W�@�h�\��� */
                    {
                        menu = mtail->menu;
                        continue;
                    }
#endif

                    break;
                }
                if (mlevel && !(mlevel & level))
                    continue;
                if (!strncmp(mtail->desc, OPT_OPERATOR, strlen(OPT_OPERATOR)) && !(supervisor || !str_cmp(cuser.userid, ELDER) || !str_cmp(cuser.userid, STR_SYSOP)))
                    continue;

                table[++max] = mtail;
            }

            mmx = BMAX(mmx, max);

            if ((menu == menu_main) && (cutmp->ufo & UFO_BIFF))
                cmd = 'M';
            else if ((menu == menu_main) && (cutmp->ufo & UFO_BIFFN))
                cmd = 'U';
            else
                cmd = mlevel ^ PERM_MENU;  /* default command */
            keyboard_cmd = false;
            utmp_mode(mtail->umode);
        }

        if (mode & MENU_DRAW)
        {
            max_item_length = 0;

            if (mode & MENU_FILM)
            {
                clear();
            }
            vs_head(mtail->desc, NULL);
            //prints("\n\x1b[30;47m     �ﶵ         �ﶵ����                         �ʺA�ݪO                   \x1b[m\n");
            for (int i = 0; i <= mmx; i++)
            {
                int yi, xi;
                if (height > 0 && width > 0)
                {
                    yi = y + (i % height);
                    xi = x + (i / height * width);
                }
                else
                {
                    yi = y + i;
                    xi = x;
                }
                move_ansi(yi, xi + 2);
                if (i <= max)
                {
                    char item[ANSILINELEN];
                    const MENU *const mptr = table[i];
                    const char *const str = check_info(mptr->desc);
                    int match_max = BMIN(cmdcur_max, strcspn(str, "\n"));
                    sprintf(item, "\x1b[m(\x1b[1;36m%-*.*s\x1b[m)%.*s",
                            cmdcur_max, match_max, str, strcspn(str + match_max, "\n"), str + match_max);
                    outs(item);

                    if (HAVE_UFO2_CONF(UFO2_MENU_LIGHTBAR))
                        grayout(yi, yi + 1, GRAYOUT_COLORNORM);

                    item_length[i] = 4 + strip_ansi_len(str);
                    max_item_length = BMAX(max_item_length, item_length[i]);
                }
                clrtoeol();
            }
            if (mode & MENU_FILM)
            {
                if (!MENU_NOMOVIE_POS(y, x))
                    movie();
                menu_foot();
                cx = -1;
            }

            mmx = max;
            mode = 0;
        }

        /* Invoke hotkey functions for keyboard input only */
        switch ((keyboard_cmd) ? cmd : KEY_NONE)
        {
        case KEY_PGUP:
            cc = (cc == 0) ? max : 0;
        break;

        case KEY_PGDN:
            cc = (cc == max) ? 0 : max;
        break;

        case KEY_DOWN:
            if (++cc <= max)
                break;
            // Else falls through
            //    to wrap around cursor

        case KEY_HOME:
            cc = 0;
            break;

        case KEY_UP:
            if (--cc >= 0)
                break;
            // Else falls through
            //    to wrap around cursor

        case KEY_END:
            cc = max;
            break;

        case KEY_RIGHT:
            if (height > 0 && cc + height <= max)
            {
                cc += height;
                break;
            }
            // Else falls through
            //    to execute the function

        case '\n':
            {
                MENU *const mptr = table[cc];
                MenuItem mitem = mptr->item;
                int mmode = mptr->umode;
                int res;
#if !NO_SO
                /* Thor.990212: dynamic load, with negative umode */
                if (mmode < 0)
                {
                    mitem.func = (int (*)(void)) DL_GET(mitem.dlfunc);
                    if (!mitem.func) break;
                    mmode = -mmode;
  #ifndef DL_HOTSWAP
                    mptr->item = mitem;
                    mptr->umode = mmode;
  #endif
                }
#endif
                utmp_mode(mmode & M_MASK /* = mptr->umode*/);

                if ((mmode & M_MASK) <= M_XMENU)
                {
                    if (cmdcur_max == 1)
                        mtail->level = PERM_MENU + mptr->desc[0];
                    menu = mitem.menu;
                    mode = MENU_LOAD | MENU_DRAW | MENU_FILM;
                    continue;
                }

                if (mmode & M_ARG)
                    res = (*mitem.funcarg.func)(mitem.funcarg.arg);
                else
                    res = (*mitem.func)();

                utmp_mode(mtail->umode);

                switch (res)
                {
                case XEASY:
#if 1
                    /* Thor.980826: �� outz �N���� move + clrtoeol�F */
                    outf(footer);
#endif
                    mode = 0;
                    break;

                case QUIT:
                    return;

#ifdef  HAVE_CHANGE_SKIN
                case SKIN:
                    vmsg("DEBUG:SKIN");
                    vmsg("123");
                    //(*s_menu)();
                    return;
                    vmsg("1234");
                    break;
#endif

                default:
                    mode = MENU_DRAW | MENU_FILM;
                }

                cmd = ' ';
                keyboard_cmd = false;
                continue;
            }

#ifdef EVERY_Z
        case Ctrl('Z'):
            every_Z();          /* Thor: ctrl-Z everywhere */
            goto menu_key;
#endif
        case Ctrl('U'):
            every_U();
            break;
        case Ctrl('B'):
            every_B();
            break;
        case Ctrl('S'):
        case 'S':
        case 's':
        case '/':
            every_S();
            break;
        case KEY_LEFT:
            if (height > 0 && cc - height >= 0)
            {
                cc -= height;
                break;
            }
            // Else falls through
            //    to enter the parent menu

        case KEY_ESC:
        case Meta(KEY_ESC):
        case 'e':
            if (menu != menu_main)
            {
                mtail->level = PERM_MENU + table[cc]->desc[0];
                menu = mtail->item.menu;
                mode = MENU_LOAD | MENU_DRAW | MENU_FILM;
                continue;
            }

            cmd = 'G';
            keyboard_cmd = false;

            // Falls through
            //    to move the cursor to option 'G' ('Goodbye'; exiting BBS)

            /* Command matching */
        default:
            switch (cmd)
            {
            default:
                {
                    int maxlen = 0;

                    cmd = tolower(cmd);

                    /* IID.20200107: Match input sequence. */
                    for (int i = 0; i <= max; i++)
                    {
                        const char *const mdesc = table[i]->desc;
                        int match_max = BMIN(cmdcur_max, strlen(mdesc));
                        /* Skip spaces */
                        cmdcur[i] += strspn(mdesc + cmdcur[i], " ");
                        /* Not matched or cursor reached the end */
                        if (cmdcur[i] >= match_max
                            || tolower(mdesc[cmdcur[i]]) != cmd)
                        {
                            /* Reset and skip spaces */
                            cmdcur[i] = strspn(mdesc + cmdcur[0], " ");
                            cmdlen[i] = 0;
                        }
                        if (tolower(mdesc[cmdcur[i]]) == cmd)
                        {
                            cmdcur[i]++;
                            cmdlen[i]++;
                        }
                        if (cmdlen[i] > maxlen)
                        {
                            maxlen = cmdlen[i];
                            cc = i;
                        }
                    }
                }

                // Falls through
                //    to keep the input

            case ' ':  /* Ignore space for matching */
                if (keyboard_cmd)  /* `cmd` is from keyboard */
                    keep_cmd = true;
            }
        }

        {
            int ycc, xcc, ycx, xcx;
            if (height > 0 && width > 0)
            {
                ycc = y + (cc % height);
                xcc = x + (cc / height * width);
                ycx = y + (cx % height);
                xcx = x + (cx / height * width);
            }
            else
            {
                ycc = y + cc;
                xcc = x;
                ycx = y + cx;
                xcx = x;
            }
            if (cc != cx)
            {
                const char *explan = strchr(table[cc]->desc, '\n');
                if (!explan)
                    explan = strchr(mtail->desc, '\n');
                if (explan)
                {
                    move(b_lines - 1, b_cols - strip_ansi_len(explan + 1));
                    outs(explan + 1);
                }

                if (cx >= 0)
                {
                    cursor_bar_clear(ycx, xcx, width);
                }
                cursor_bar_show(ycc, xcc, width);
                cx = cc;
            }
            else
            {
                move(ycc, xcc + 1);
            }
        }

menu_key:

        cmd = vkey();
        keyboard_cmd = true;

        if (gety_ref(y_ref) != y || getx_ref(x_ref) != x || gety_ref(height_ref) != height || getx_ref(width_ref) != width)
        {
            y = gety_ref(y_ref);
            x = getx_ref(x_ref);
            height = gety_ref(height_ref);
            width = getx_ref(width_ref);

            mode = MENU_LOAD | MENU_DRAW | MENU_FILM;
        }
    }
}

