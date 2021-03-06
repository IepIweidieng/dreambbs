/*-------------------------------------------------------*/
/* innbbs.c     ( NTHU CS MapleBBS Ver 3.10 )            */
/*-------------------------------------------------------*/
/* target : 轉信設定                                     */
/* create : 04/04/25                                     */
/* update :   /  /                                       */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"


typedef struct {
    int (*item_func)(XO *xo, int pos);
    void (*query_func)(const void *obj);
    int (*add_func)(const char *fpath, const void *old, int pos);
    int (*sync_func)(const void *lhs, const void *rhs);
    bool (*search_func)(const void *obj, const char *key);
    bool dirty;
} InnbbsXyz;


/* ----------------------------------------------------- */
/* nodelist.bbs 子函式                                   */
/* ----------------------------------------------------- */


static int
nl_item(
    XO *xo,
    int pos)
{
    const nodelist_t *nl = (const nodelist_t *)xo_pool_base + pos;
    const int num = pos + 1;
    prints("%6d %-13s%-*.*s %s(%d)\n", num,
        nl->name, d_cols + 45, d_cols + 45, nl->host, nl->xmode & INN_USEIHAVE ? "IHAVE" : "POST", nl->port);
    return XO_NONE;
}


static void
nl_query(
    const void *nl_obj)
{
    const nodelist_t *nl = (const nodelist_t *)nl_obj;

    move(3, 0);
    clrtobot();
    prints("\n\n轉信站台：%s\n站台位址：%s\n站台協定：%s(%d)\n被 餵 信：%s",
        nl->name, nl->host, nl->xmode & INN_USEIHAVE ? "IHAVE" : "POST", nl->port, nl->xmode & INN_FEEDED ? "是" : "否");
    vmsg(NULL);
}


static int      /* 1:成功 0:失敗 */
nl_add(
    const char *fpath,
    const void *nl_old,
    int pos)
{
    const nodelist_t *old = (const nodelist_t *)nl_old;
    nodelist_t nl;
    int ch, port;
    char ans[8];
    char msg1[] = "協定：(1)IHAVE (2)POST [1] ";
    char msg2[] = "此站台會主動餵信給本站嗎(y/N)？[N] ";

    if (old)
        memcpy(&nl, old, sizeof(nodelist_t));
    else
        memset(&nl, 0, sizeof(nodelist_t));

    if (vget(B_LINES_REF, 0, "英文站名：", nl.name, sizeof(nl.name), GCARRY) &&
        vget(B_LINES_REF, 0, "站址：", nl.host, /* sizeof(nl.host) */ 70, GCARRY))
    {
        msg1[24] = (nl.xmode & INN_USEPOST) ? '2' : '1';        /* 新增資料預設 INN_HAVE */
        ch = vans(msg1);
        if (ch != '1' && ch != '2')
            ch = msg1[24];

        if (ch == '1')
        {
            nl.xmode = INN_USEIHAVE | INN_FEEDED;       /* IHAVE 一定是被餵信 */
            vget(B_LINES_REF, 0, "Port：[7777] ", ans, 6, DOECHO);
            if ((port = atoi(ans)) <= 0)
                port = 7777;
        }
        else /* if (ch == '2') */
        {
            nl.xmode = INN_USEPOST;
            vget(B_LINES_REF, 0, "Port：[119] ", ans, 6, DOECHO);
            if ((port = atoi(ans)) <= 0)
                port = 119;

            msg2[32] = (old && old->xmode & INN_FEEDED) ? 'Y' : 'N';    /* 新增資料預設不餵信 */
            ch = vans(msg2);
            if (ch != 'y' && ch != 'n')
                ch = msg2[32] | 0x20;

            if (ch == 'y')
                nl.xmode |= INN_FEEDED;
        }
        nl.port = port;

        if (old)
            rec_put(fpath, &nl, sizeof(nodelist_t), pos);
        else
            rec_add(fpath, &nl, sizeof(nodelist_t));
        return 1;
    }
    return 0;
}


static int
nl_cmp(
    const void *a, const void *b)
{
    /* 依 name 排序 */
    return str_casecmp(((const nodelist_t *)a) -> name, ((const nodelist_t *)b) -> name);
}


static bool
nl_search(
    const void *nl,
    const char *key)
{
    return str_casestr(((const nodelist_t *)nl) -> name, key) || str_casestr(((const nodelist_t *)nl) -> host, key);
}


/* ----------------------------------------------------- */
/* newsfeeds.bbs 子函式                                  */
/* ----------------------------------------------------- */


static int
nf_item(
    XO *xo,
    int pos)
{
    const newsfeeds_t *nf = (const newsfeeds_t *)xo_pool_base + pos;
    const int num = pos + 1;

    int bno;
    BRD *brd;
    char outgo, income;

    if ((bno = brd_bno(nf->board)) >= 0)
    {
        if (nf->xmode & INN_ERROR)
        {
            outgo = income = '?';
        }
        else
        {
            brd = bshm->bcache + bno;
            outgo = brd->battr & BRD_NOTRAN ? ' ' : '<';
            income = nf->xmode & INN_NOINCOME ? ' ': '>';
        }
    }
    else
    {
        outgo = income = 'X';
    }

    prints("%6d %-13s%-*.*s %c-%c %-*s %.7s\n", num,
        nf->path, d_cols + 33, d_cols + 33, nf->newsgroup, outgo, income, IDLEN, nf->board, nf->charset);

    return XO_NONE;
}


static void
nf_query(
    const void *nf_obj)
{
    const newsfeeds_t *nf = (const newsfeeds_t *)nf_obj;
    nodelist_t nl;
    int fd;
    int rc = 0;
    BRD *brd;
    const char *outgo, *income;

    /* 找出該站台在 nodelist.bbs 中的資訊 */
    if ((fd = open("innd/nodelist.bbs", O_RDONLY)) >= 0)
    {
        while (read(fd, &nl, sizeof(nodelist_t)) == sizeof(nodelist_t))
        {
            if (!strcmp(nl.name, nf->path))
            {
                rc = 1;
                break;
            }
        }
        close(fd);
    }
    if (!rc)
    {
        memset(&nl, 0, sizeof(nodelist_t));
        strcpy(nl.host, "\x1b[1;33m此站台不在 nodelist.bbs 中\x1b[m");
    }

    /* 看板狀態 */
    if ((rc = brd_bno(nf->board)) >= 0)
    {
        brd = bshm->bcache + rc;
        outgo = brd->battr & BRD_NOTRAN ? "\x1b[1;33m不轉出\x1b[m"  : "轉出";
        income = nf->xmode & INN_NOINCOME ? "且\x1b[1;33m不轉進\x1b[m" : "且轉進";
    }
    else
    {
        outgo = "\x1b[1;33m此看板不存在\x1b[m";
        income = "";
    }

    move(3, 0);
    clrtobot();
    prints("\n\n轉信站台：%s\n站台位址：%s\n站台協定：%s(%d)\n"
        "轉信群組：%s%s\n本站看板：%s (%s%s)\n使用字集：%s",
        nf->path, nl.host, nl.xmode & INN_USEIHAVE ? "IHAVE" : "POST", nl.port,
        nf->newsgroup, nf->xmode & INN_ERROR ? " (\x1b[1;33m此群組不存在\x1b[m)" : "",
        nf->board, outgo, income, nf->charset);
    if (rc && !(nl.xmode & INN_FEEDED))
        prints("\n目前篇數：%d", nf->high);
    vmsg(NULL);
}


static int      /* 1:成功 0:失敗 */
nf_add(
    const char *fpath,
    const void *nf_old,
    int pos)
{
    const newsfeeds_t *old = (const newsfeeds_t *)nf_old;
    newsfeeds_t nf;
    int high;
    char ans[12];
    BRD *brd;

    if (old)
        memcpy(&nf, old, sizeof(newsfeeds_t));
    else
    {
        memset(&nf, 0, sizeof(newsfeeds_t));
        nf.high = INT_MAX;              /* 第一次取信強迫 reload */
    }

    if ((brd = ask_board(nf.board, BRD_R_BIT, NULL)) &&
        vget(B_LINES_REF, 0, "英文站名：", nf.path, sizeof(nf.path), GCARRY) &&
        vget(B_LINES_REF, 0, "群組：", nf.newsgroup, /*  sizeof(nf.newsgroup) */ 70, GCARRY))
    {
        if (!vget(B_LINES_REF, 0, "字集 [big5]：", nf.charset, sizeof(nf.charset), GCARRY))
            str_scpy(nf.charset, "big5", sizeof(nf.charset));
        nf.xmode = (vans("是否轉進(Y/n)？[Y] ") == 'n') ? INN_NOINCOME : 0;

        if (vans("是否更改轉信的 high-number 設定，這設定對被餵信的群組無效(y/N)？[N] ") == 'y')
        {
            sprintf(ans, "%d", nf.high);
            vget(B_LINES_REF, 0, "目前篇數：", ans, 11, GCARRY);
            if ((high = atoi(ans)) >= 0)
                nf.high = high;
        }


        if (old)
            rec_put(fpath, &nf, sizeof(newsfeeds_t), pos);
        else
            rec_add(fpath, &nf, sizeof(newsfeeds_t));

        if ((brd->battr & BRD_NOTRAN) && vans("本板屬性目前為不轉出，是否改為轉出(Y/n)？[Y] ") != 'n')
        {
            high = brd - bshm->bcache;
            brd->battr &= ~BRD_NOTRAN;
            rec_put(FN_BRD, brd, sizeof(BRD), high);
        }

        return 1;
    }
    return 0;
}


static int
nf_cmp(
    const void *a, const void *b)
{
    /* path/newsgroup 交叉比對 */
    int k = str_casecmp(((const newsfeeds_t *)a) -> path, ((const newsfeeds_t *)b) -> path);
    return k ? k : str_casecmp(((const newsfeeds_t *)a) -> newsgroup, ((const newsfeeds_t *)b) -> newsgroup);
}


static bool
nf_search(
    const void *nf,
    const char *key)
{
    return str_casestr(((const newsfeeds_t *)nf) -> newsgroup, key) || str_casestr(((const newsfeeds_t *)nf) -> board, key);
}


/* ----------------------------------------------------- */
/* ncmperm.bbs 子函式                                    */
/* ----------------------------------------------------- */


static int
ncm_item(
    XO *xo,
    int pos)
{
    const ncmperm_t *ncm = (const ncmperm_t *)xo_pool_base + pos;
    const int num = pos + 1;
    prints("%6d %-*.*s%-23.23s %s\n", num,
        d_cols + 46, d_cols + 46, ncm->issuer, ncm->type, ncm->perm ? "○" : "╳");
    return XO_NONE;
}


static void
ncm_query(
    const void *ncm_obj)
{
    const ncmperm_t *ncm = (const ncmperm_t *)ncm_obj;

    move(3, 0);
    clrtobot();
    prints("\n\n發行站台：%s\n砍信種類：%s\n允許\砍信：%s",
        ncm->issuer, ncm->type, ncm->perm ? "○" : "╳");
    vmsg(NULL);
}


static int      /* 1:成功 0:失敗 */
ncm_add(
    const char *fpath,
    const void *ncm_old,
    int pos)
{
    const ncmperm_t *old = (const ncmperm_t *)ncm_old;
    ncmperm_t ncm;

    if (old)
        memcpy(&ncm, old, sizeof(ncmperm_t));
    else
        memset(&ncm, 0, sizeof(ncmperm_t));

    if (vget(B_LINES_REF, 0, "發行：", ncm.issuer, /* sizeof(ncm.issuer) */ 70, GCARRY) &&
        vget(B_LINES_REF, 0, "種類：", ncm.type, sizeof(ncm.type), GCARRY))
    {
        ncm.perm = (vans("允許\此 NCM message 砍信(y/N)？[N] ") == 'y');

        if (old)
            rec_put(fpath, &ncm, sizeof(ncmperm_t), pos);
        else
            rec_add(fpath, &ncm, sizeof(ncmperm_t));
        return 1;
    }
    return 0;
}


static int
ncm_cmp(
    const void *a, const void *b)
{
    /* issuer/type 交叉比對 */
    int k = str_casecmp(((const ncmperm_t *)a) -> issuer, ((const ncmperm_t *)b) -> issuer);
    return k ? k : str_casecmp(((const ncmperm_t *)a) -> type, ((const ncmperm_t *)b) -> type);
}


static bool
ncm_search(
    const void *ncm,
    const char *key)
{
    return str_casestr(((const ncmperm_t *)ncm) -> issuer, key) || str_casestr(((const ncmperm_t *)ncm) -> type, key);
}


/* ----------------------------------------------------- */
/* spamrule.bbs 子函式                                   */
/* ----------------------------------------------------- */


static const char *
spam_compare(
    int xmode)
{
    if (xmode & INN_SPAMADDR)
        return "作者";
    if (xmode & INN_SPAMNICK)
        return "暱稱";
    if (xmode & INN_SPAMSUBJECT)
        return "標題";
    if (xmode & INN_SPAMPATH)
        return "路徑";
    if (xmode & INN_SPAMMSGID)
        return "MSID";
    if (xmode & INN_SPAMBODY)
        return "本文";
    if (xmode & INN_SPAMSITE)
        return "組織";
    if (xmode & INN_SPAMPOSTHOST)
        return "來源";
    return "？？";
}


static int
spam_item(
    XO *xo,
    int pos)
{
    const spamrule_t *spam = (const spamrule_t *)xo_pool_base + pos;
    const int num = pos + 1;

    const char *path, *board;

    path = spam->path;
    board = spam->board;
    prints("%6d %-13s%-*s [%s] 包含 %.*s\n",
        num, *path ? path : "所有站台", IDLEN, *board ? board : "所有看板",
        spam_compare(spam->xmode), d_cols + 31, spam->detail);

    return XO_NONE;
}


static void
spam_query(
    const void *spam_obj)
{
    const spamrule_t *spam = (const spamrule_t *)spam_obj;
    const char *path, *board;

    path = spam->path;
    board = spam->board;

    move(3, 0);
    clrtobot();
    prints("\n\n適用站台：%s\n適用看板：%s\n比較項目：%s\n比較內容：%s",
        *path ? path : "所有站台", *board ? board : "所有看板", spam_compare(spam->xmode), spam->detail);
    vmsg("若滿足此規則，會被視為廣告而無法轉信進來");
}


static int      /* 1:成功 0:失敗 */
spam_add(
    const char *fpath,
    const void *spam_old,
    int pos)
{
    const spamrule_t *old = (const spamrule_t *)spam_old;
    spamrule_t spam;

    if (old)
        memcpy(&spam, old, sizeof(spamrule_t));
    else
        memset(&spam, 0, sizeof(spamrule_t));

    vget(B_LINES_REF, 0, "英文站名：", spam.path, sizeof(spam.path), GCARRY);
    ask_board(spam.board, BRD_W_BIT, NULL);

    switch (vans("擋信規則 1)作者 2)暱稱 3)標題 4)路徑 5)MSGID 6)本文 7)組織 8)來源 [Q] "))
    {
    case '1':
        spam.xmode = INN_SPAMADDR;
        break;
    case '2':
        spam.xmode = INN_SPAMNICK;
        break;
    case '3':
        spam.xmode = INN_SPAMSUBJECT;
        break;
    case '4':
        spam.xmode = INN_SPAMPATH;
        break;
    case '5':
        spam.xmode = INN_SPAMMSGID;
        break;
    case '6':
        spam.xmode = INN_SPAMBODY;
        break;
    case '7':
        spam.xmode = INN_SPAMSITE;
        break;
    case '8':
        spam.xmode = INN_SPAMPOSTHOST;
        break;
    default:
        return 0;
    }

    if (vget(B_LINES_REF, 0, "包含：", spam.detail, /* sizeof(spam.detail) */ 70, GCARRY))
    {
        if (old)
            rec_put(fpath, &spam, sizeof(spamrule_t), pos);
        else
            rec_add(fpath, &spam, sizeof(spamrule_t));
        return 1;
    }
    return 0;
}


static int
spam_cmp(
    const void *a, const void *b)
{
    const spamrule_t *x = (const spamrule_t *)a;
    const spamrule_t *y = (const spamrule_t *)b;
    /* path/board/xmode/detail 交叉比對 */
    int i = strcmp(x->path, y->path);
    int j = strcmp(x->board, y->board);
    int k = x->xmode - y->xmode;
    return i ? i : j ? j : k ? k : str_casecmp(x->detail, y->detail);
}


static bool
spam_search(
    const void *spam,
    const char *key)
{
    return str_casestr(((const spamrule_t *)spam) -> detail, key);
}


/* ----------------------------------------------------- */
/* 轉信設定主函式                                        */
/* ----------------------------------------------------- */

/* IID.20191223: Rewrite with `xover()` */

static int
innbbs_foot(
    XO *xo)
{
    outf(FEETER_INNBBS);
    return XO_NONE;
}

static int
innbbs_cur(
    XO *xo,
    int pos)
{
    InnbbsXyz *xyz = (InnbbsXyz *)xo->xyz;
    move(3 + pos - xo->top, 0);
    return xyz->item_func(xo, pos);
}

static int
innbbs_body(
    XO *xo)
{
    InnbbsXyz *xyz = (InnbbsXyz *)xo->xyz;
    int num, max, tail;

    move(3, 0);

    max = xo->max;
    if (max <= 0)
    {
        outs("\n《轉信設定》目前沒有資料\n");
        outs("\n  (^P)新增資料\n");
        clrtobot();
        return innbbs_foot(xo);
    }

    num = xo->top;
    tail = num + XO_TALL;
    max = BMIN(max, tail);

    do
    {
        xyz->item_func(xo, num++);
    } while (num < max);

    clrtobot();
    return innbbs_foot(xo);
}

static int
innbbs_head(
    XO *xo)
{
    vs_head("轉信設定", str_site);
    prints(NECKINNBBS, d_cols, "");
    return innbbs_body(xo);
}

static int
innbbs_init(
    XO *xo)
{
    xo_load(xo, xo->recsiz);
    return innbbs_head(xo);
}

static int
innbbs_load(
    XO *xo)
{
    xo_load(xo, xo->recsiz);
    return innbbs_body(xo);
}

static int
innbbs_query(
    XO *xo,
    int pos)
{
    InnbbsXyz *xyz = (InnbbsXyz *)xo->xyz;
    xyz->query_func(xo_pool_base + pos * xo->recsiz);
    return XO_BODY;
}

static int
innbbs_add(
    XO *xo)
{
    InnbbsXyz *xyz = (InnbbsXyz *)xo->xyz;
    if (xyz->add_func(xo->dir, NULL, -1))
    {
        xyz->dirty = true;
        return XO_INIT;
    }
    return XO_HEAD;
}

static int
innbbs_del(
    XO *xo,
    int pos)
{
    InnbbsXyz *xyz = (InnbbsXyz *)xo->xyz;
    if (vans(msg_del_ny) == 'y')
    {
        rec_del(xo->dir, xo->recsiz, pos, NULL, NULL);
        xyz->dirty = true;
        return XO_LOAD;
    }
    return XO_FOOT;
}

static int
innbbs_edit(
    XO *xo,
    int pos)
{
    InnbbsXyz *xyz = (InnbbsXyz *)xo->xyz;
    if (xyz->add_func(xo->dir, xo_pool_base + pos * xo->recsiz, pos))
    {
        xyz->dirty = true;
        return XO_INIT;
    }
    return XO_HEAD;
}

static int
innbbs_search(
    XO *xo,
    int pos)
{
    InnbbsXyz *xyz = (InnbbsXyz *)xo->xyz;
    char buf[40];
    int i, num = xo->max;

    if (vget(B_LINES_REF, 0, "關鍵字：", buf, sizeof(buf), DOECHO))
    {
        str_lower(buf, buf);
        for (i = pos + 1; i <= num; i++)
        {
            if (xyz->search_func(xo_pool_base + i * xo->recsiz, buf))
            {
                return XR_FOOT + XO_MOVE + i;
            }
        }
    }
    return XO_FOOT;
}

static int
innbbs_help(
    XO *xo)
{
    more("etc/innbbs.hlp", (char *) -1);
    return XO_HEAD;
}

static KeyFuncList innbbs_cb =
{
    {XO_INIT, {innbbs_init}},
    {XO_LOAD, {innbbs_load}},
    {XO_HEAD, {innbbs_head}},
    {XO_BODY, {innbbs_body}},
    {XO_FOOT, {innbbs_foot}},
    {XO_CUR | XO_POSF, {.posf = innbbs_cur}},


    {' ' | XO_POSF, {.posf = innbbs_query}},
    {'r' | XO_POSF, {.posf = innbbs_query}},

    {Ctrl('P'), {innbbs_add}},
    {'d' | XO_POSF, {.posf = innbbs_del}},
    {'E' | XO_POSF, {.posf = innbbs_edit}},

    {'/' | XO_POSF, {.posf = innbbs_search}},

    {'h', {innbbs_help}}
};

int
a_innbbs(void)
{
    DL_HOLD;
    XO *xo, *last;
    InnbbsXyz xyz;
    const char *fpath;
    int recsiz;

    if (!check_admin(cuser.userid) && str_casecmp(cuser.userid, SYSOPNAME))
    {
        vmsg("◎ 你不是系統管理員！");
        return DL_RELEASE(0);
    }


    vs_bar("轉信設定");

    more("etc/innbbs.hlp", (char *) -1);

    switch (vans("請選擇 1)轉文站台列表 2)轉文看板列表 3)NoCeM擋文規則 4)廣告文名單：[Q] "))
    {
    case '1':
        fpath = "innd/nodelist.bbs";
        recsiz = sizeof(nodelist_t);
        xyz.item_func = nl_item;
        xyz.query_func = nl_query;
        xyz.add_func = nl_add;
        xyz.sync_func = nl_cmp;
        xyz.search_func = nl_search;
        break;

    case '2':
        fpath = "innd/newsfeeds.bbs";
        recsiz = sizeof(newsfeeds_t);
        xyz.item_func = nf_item;
        xyz.query_func = nf_query;
        xyz.add_func = nf_add;
        xyz.sync_func = nf_cmp;
        xyz.search_func = nf_search;
        break;

    case '3':
        fpath = "innd/ncmperm.bbs";
        recsiz = sizeof(ncmperm_t);
        xyz.item_func = ncm_item;
        xyz.query_func = ncm_query;
        xyz.add_func = ncm_add;
        xyz.sync_func = ncm_cmp;
        xyz.search_func = ncm_search;
        break;

    case '4':
        fpath = "innd/spamrule.bbs";
        recsiz = sizeof(spamrule_t);
        xyz.item_func = spam_item;
        xyz.query_func = spam_query;
        xyz.add_func = spam_add;
        xyz.sync_func = spam_cmp;
        xyz.search_func = spam_search;
        break;

    default:
        return DL_RELEASE(0);
    }

    last = xz[XZ_OTHER - XO_ZONE].xo;  /* record */

    utmp_mode(M_OMENU);
    xz[XZ_OTHER - XO_ZONE].xo = xo = xo_new(fpath);
    xo->cb = innbbs_cb;
    xo->recsiz = recsiz;
    xo->xyz = &xyz;
    xo->pos = 0;

    xyz.dirty = false;

    xover(XZ_OTHER);

    if (xyz.dirty)
        rec_sync(fpath, recsiz, xyz.sync_func, NULL);

    free(xo);

    xz[XZ_OTHER - XO_ZONE].xo = last;  /* restore */
    return DL_RELEASE(0);
}
