/*-------------------------------------------------------*/
/* xover.c      ( NTHU CS MapleBBS Ver 3.02 )            */
/*-------------------------------------------------------*/
/* target : board/mail interactive reading routines      */
/* create : 95/03/29                                     */
/* update : 2000/01/02                                   */
/*-------------------------------------------------------*/

#include "bbs.h"

#define XO_STACK        (5)
#define MAX_LEVEL       (20)
static int xo_stack_level;
static int xo_user_level;

#ifdef  HAVE_FAVORITE
#define MSG_ZONE_SWITCH \
    "�ֳt�����GA)��ذ� B)�峹�C�� C)�ݪO�C�� M)�H�� F)�ڪ��̷R P)�i���\\��G"

#define MSG_ZONE_ADVANCE \
    "�i���\\��GU)�ϥΪ̦W�� W)�d�ݰT���G"

#else

#define MSG_ZONE_SWITCH \
    "�ֳt�����GA)��ذ� B)�峹�C�� C)�ݪO�C�� M)�H�� U)�ϥΪ̦W�� W)�d�ݰT���G"
#endif


/* ----------------------------------------------------- */
/* keep xover record                                     */
/* ----------------------------------------------------- */

static XO *xo_root;             /* root of overview list */


XO *
xo_new(
    const char *path)
{
    XO *xo;
    int len;

    len = strlen(path) + 1;

    xo = (XO *) malloc(SIZEOF_FLEX(XO, len));

    memcpy(xo->dir, path, len);
    xo->cb = NULL;
    xo->recsiz = 0;

    return (xo);
}


XO *
xo_get(
    const char *path)
{
    XO *xo;

    for (xo = xo_root; xo; xo = xo->nxt)
    {
        if (!strcmp(xo->dir, path))
            return xo;
    }

    xo = xo_new(path);
    xo->nxt = xo_root;
    xo_root = xo;
    xo->xyz = NULL;
    xo->pos = XO_TAIL;          /* �Ĥ@���i�J�ɡA�N��Щ�b�̫᭱ */

    return xo;
}


#if 0
void
xo_free(
    XO *xo)
{
    free(xo->xyz);
    free(xo);
}
#endif


/* ----------------------------------------------------- */
/* interactive menu routines                             */
/* ----------------------------------------------------- */


char xo_pool[(T_LINES - 4) * XO_RSIZ];  /* XO's data I/O pool */


void
xo_load(
    XO *xo,
    int recsiz)
{
    int fd, max;

    max = 0;
    if ((fd = open(xo->dir, O_RDONLY)) >= 0)
    {
        int pos, top;
        struct stat st;

        fstat(fd, &st);
        max = st.st_size / recsiz;
        if (max > 0)
        {
            pos = xo->pos;
            if (pos <= 0)
            {
                pos = top = 0;
            }
            else
            {
                pos = BMIN(pos, max - 1);
                top = (pos / XO_TALL) * XO_TALL;
            }
            xo->pos = pos;
            xo->top = top;

            lseek(fd, (off_t) (recsiz * top), SEEK_SET);
            read(fd, xo_pool, recsiz * XO_TALL);
        }
        close(fd);
    }

    xo->max = max;
}


/* static */
void
xo_fpath(
    char *fpath,
    const char *dir,
    HDR *hdr)
{
    hdr_fpath(fpath, dir, hdr);
}


/* ----------------------------------------------------- */
/* nhead:                                                */
/* 0 ==> �̾� TagList �s��R��                           */
/* !0 ==> �̾� range [nhead, ntail] �R��                 */
/* ----------------------------------------------------- */
/* notice : *.n - new file                               */
/* *.o - old file                                        */
/* ----------------------------------------------------- */


int
hdr_prune(
    const char *folder,
    int nhead, int ntail,
    int post)
{
    int count, fdr, fsize, xmode, cancel, dmode GCC_UNUSED;
    HDR *hdr;
    FILE *fpw;
    char fnew[80], fold[80];

    if ((fdr = open(folder, O_RDONLY)) < 0)
        return -1;

    if (!(fpw = f_new(folder, fnew)))
    {
        close(fdr);
        return -1;
    }

    xmode = *folder;
    cancel = (xmode == 'b');
    dmode = (xmode == 'u') ? 0 : (POST_CANCEL | POST_DELETE | POST_MDELETE);

    fsize = count = 0;
    mgets(-1);
    while ((hdr = (HDR *) mread(fdr, sizeof(HDR))))
    {
        xmode = hdr->xmode;
        count++;
#if 0
        if (xmode & dmode)              /* �w�R�� */
                continue;
#endif
        if (((xmode & (POST_MARKED|POST_LOCK|POST_DELETE)) ||   /* �аO */
                (nhead && (count < nhead || count > ntail)) ||  /* range */
            (!nhead && Tagger(hdr->chrono, count - 1, TAG_NIN)))        /* TagList */
            && !(post == 3 && (hdr->xmode & POST_DELETE)))
        {
            if (!post)
            {
                if ((fwrite(hdr, sizeof(HDR), 1, fpw) != 1))
                {
                    close(fdr);
                    fclose(fpw);
                    unlink(fnew);
                    return -1;
                }
                fsize++;
            }
        }
        else
        {
            /* �Y���ݪO�N�s�u��H */
            if (cancel)
                cancel_post(hdr);
            if (!post)
            {

                hdr_fpath(fold, folder, hdr);
                unlink(fold);
            }
            else
            {
                if (post == 1)
                {
                    hdr->xmode |= POST_MDELETE;
                    sprintf(hdr->title, "<< ���峹�� %s �R�� >>", cuser.userid);
                }
#ifdef  HAVE_MAILUNDELETE
                else if (post == 3 && (hdr->xmode & POST_DELETE))
                {
                    hdr_fpath(fold, folder, hdr);
                    unlink(fold);
                }
                else if (post != 3)
#else
                else
#endif
                    hdr->xmode |= POST_DELETE;
            }
        }
#ifdef  HAVE_MAILUNDELETE
        if (post && !(post == 3 && (hdr->xmode & POST_DELETE)))
#else
        if (post)
#endif
        {
            if ((fwrite(hdr, sizeof(HDR), 1, fpw) != 1))
            {
                close(fdr);
                fclose(fpw);
                unlink(fnew);
                return -1;
            }
            fsize++;
        }
    }
    close(fdr);
    fclose(fpw);

    sprintf(fold, "%s.o", folder);
    rename(folder, fold);
    if (fsize)
        rename(fnew, folder);
    else
        unlink(fnew);

    return 0;
}


int
xo_delete(
    XO *xo)
{
    char buf[8];
    int head, tail;

    if ((bbsmode == M_READA) && !(bbstate & STAT_BOARD))
        return XO_NONE;

    vget(B_LINES_REF, 0, "[�]�w�R���d��] �_�I�G", buf, 6, DOECHO);
    head = atoi(buf);
    if (head <= 0)
    {
        zmsg("�_�I���~");
        return XO_FOOT;
    }

    vget(B_LINES_REF, 28, "���I�G", buf, 6, DOECHO);
    tail = atoi(buf);
    if (tail < head)
    {
        zmsg("���I���~");
        return XO_FOOT;
    }


    if (vget(B_LINES_REF, 41, msg_sure_ny, buf, 3, LCECHO) == 'y')
    {
        if (bbsmode == M_READA)
            hdr_prune(xo->dir, head, tail, 0);
#ifdef  HAVE_MAILUNDELETE
        else if (bbsmode == M_RMAIL)
            hdr_prune(xo->dir, head, tail, 2);
#endif
        else
            hdr_prune(xo->dir, head, tail, 0);

        return XO_LOAD;
    }
    return XO_FOOT;
}


/* ----------------------------------------------------- */
/* Tag List ����                                         */
/* ----------------------------------------------------- */


int TagNum;                     /* tag's number */
TagItem TagList[TAG_MAX];       /* ascending list */


int
Tagger(
    time_t chrono,
    int recno,
    int op)                     /* op : TAG_NIN / TOGGLE / INSERT */
/* ----------------------------------------------------- */
/* return 0 : not found / full                           */
/* 1 : add                                               */
/* -1 : remove                                           */
/* ----------------------------------------------------- */
{
    int head, tail, pos=0, cmp;
    TagItem *tagp;

    for (head = 0, tail = TagNum - 1, tagp = TagList, cmp = 1; head <= tail;)
    {
        pos = (head + tail) / 2U;
        cmp = tagp[pos].chrono - chrono;
        if (!cmp)
        {
            break;
        }
        else if (cmp < 0)
        {
            head = pos + 1;
        }
        else
        {
            tail = pos - 1;
        }
    }

    if (op == TAG_NIN)
    {
        if (!cmp && recno)              /* �����Y�ԡG�s recno �@�_��� */
            cmp = recno - tagp[pos].recno;
        return cmp;
    }

    tail = TagNum;

    if (!cmp)
    {
        if (op != TAG_TOGGLE)
            return false;

        TagNum = --tail;
        memmove(&tagp[pos], &tagp[pos + 1], (tail - pos) * sizeof(TagItem));
        return -1;
    }

    if (tail < TAG_MAX)
    {
        TagItem buf[TAG_MAX];

        TagNum = tail + 1;
        tail = (tail - head) * sizeof(TagItem);
        tagp += head;
        memcpy(buf, tagp, tail);
        tagp->chrono = chrono;
        tagp->recno = recno;
        memcpy(++tagp, buf, tail);
        return true;
    }

    /* TagList is full */

    bell();
    return 0;
}


void
EnumTagHdr(
    HDR *hdr,
    const char *dir,
    int locus)
{
    rec_get(dir, hdr, sizeof(HDR), TagList[locus].recno);
}


int
AskTag(
    const char *msg)
/* ----------------------------------------------------- */
/* return value :                                        */
/* -1   : ����                                           */
/* 0    : single article                                 */
/* o.w. : whole tag list                                 */
/* ----------------------------------------------------- */
{
    char buf[80];
    /* Thor.990108: �Ȥ����j */
    /* char buf[100]; */
    int num;

    num = TagNum;
    sprintf(buf, "�� %s A)rticle T)ag Q)uit�H[%c] ", msg, num ? 'T' : 'A');
    switch (vans(buf))
    {
    case 'q':
        return -1;

    case 'a':
        return 0;
    }
    return num;
}


/* ----------------------------------------------------- */
/* tag articles according to title / author              */
/* ----------------------------------------------------- */


static int
xo_tag(
    XO *xo,
    int op)
{
    int fsize, count;
    char *fimage;
    const char *token;
    const HDR *head, *tail;

    fimage = f_map(xo->dir, &fsize);
    if (fimage == (char *) -1)
        return XO_NONE;

    head = (const HDR *) xo_pool + (xo->pos - xo->top);
    if (op == Ctrl('A'))
    {
        token = head->owner;
        op = 0;
    }
    else
    {
        token = str_ttl(head->title);
        op = 1;
    }

    head = (const HDR *) fimage;
    tail = (const HDR *) (fimage + fsize);

    count = 0;

    do
    {
        if (!strcmp(token, op ? str_ttl(head->title) : head->owner))
        {
            if (!Tagger(head->chrono, count, TAG_INSERT))
                break;
        }
        count++;
    } while (++head < tail);

    munmap(fimage, fsize);
    return XO_BODY;
}


static int
xo_prune(
    XO *xo)
{
    int num;
    char buf[80];

    if (!(num = TagNum) || ((bbsmode == M_READA) && !(bbstate & STAT_BOARD)))
        return XO_NONE;

    sprintf(buf, "�T�w�n�R�� %d �g���Ҷ�(y/N)�H[N] ", num);
    if (vans(buf) != 'y')
        return XO_FOOT;

#if 1
    /* Thor.981122: �O���R���O�� */
    sprintf(buf, "(%d)%s", num, xo->dir);
    blog("PRUNE", buf);
#endif

    if (bbsmode == M_READA)
        hdr_prune(xo->dir, 0, 0, 1);
#ifdef  HAVE_MAILUNDELETE
    else if (bbsmode == M_RMAIL)
        hdr_prune(xo->dir, 0, 0, 2);
#endif
    else
        hdr_prune(xo->dir, 0, 0, 0);

    TagNum = 0;
    return XO_LOAD;
}


/* ----------------------------------------------------- */
/* Tag's batch operation routines                        */
/* ----------------------------------------------------- */


static int
xo_copy(
    XO *xo)
{
    char fpath[128], *dir;
    HDR *hdr, xhdr;
    int tag, locus;
    FILE *fp;

    if (!cuser.userlevel)
        return XO_NONE;

    /* lkchu.990428: mat patch ��ݪ��|����w�A�ץ�copy�|�_�u�����D */
    if (bbsmode == M_READA)
    {
        /* lkchu.981205: �ɥ� tag �s��ݪ��ݩ� */
        tag = (bshm->bcache + currbno)->battr;
        if (!HAS_PERM(PERM_SYSOP) && (tag & BRD_NOFORWARD))
        {
            outz("�� ���O�峹���i��K");
            return XO_FOOT;
        }
    }

    tag = AskTag("������Ȧs��");
    if (tag < 0)
        return XO_FOOT;

    fp = tbf_open(-1);
    if (fp == NULL)
        return XO_FOOT;

    if (tag)
        hdr = &xhdr;
    else
        hdr = (HDR *) xo_pool + xo->pos - xo->top;

    locus = 0;
    dir = xo->dir;

    do
    {
        if (tag)
        {
            fputs(STR_LINE, fp);
            EnumTagHdr(hdr, dir, locus++);
        }

        // check delete or not .. by statue 2000/05/18
        if (hdr->xmode & (POST_DELETE | POST_CANCEL | POST_MDELETE))
            continue;

        if ((hdr->xmode & (POST_LOCK|GEM_RESERVED|GEM_RESTRICT)) && !(HAS_PERM(PERM_ALLBOARD) || (bbstate & STAT_BOARD)))
            continue;

        if ((hdr->xmode & (GEM_LOCK)) && !HAS_PERM(PERM_SYSOP))
            continue;


        if (!(hdr->xmode & GEM_FOLDER)) /* �d hdr �O�_ plain text */
        {
            xo_fpath(fpath, dir, hdr);
            f_suck(fp, fpath);
        }
    } while (locus < tag);

    fclose(fp);
    zmsg("��������");

    return XO_FOOT;
}


#if 0
/* Thor.981027: �� mail.c���� mail_external���N */
static inline int
rcpt_local(
    char *addr)
{
    char *str;
    int cc;

    str = addr;
    while (cc = *str++)
    {
        if (cc == '@')
        {
            /* Thor.990125: MYHOSTNAME �Τ@��J str_host */
            if (str_cmp(str, str_host))
                return 0;
            str[-1] = '\0';
            if (str = strchr(addr, '.'))
                *str = '\0';
            break;
        }
    }
    return 1;
}
#endif

#if 1
static inline int
deny_forward(void)
{
    unsigned int level;

    /* Thor.980602: �Q�N�Ҧ��ʺA�v�������ܲΤ@���login�B, �Pı�������
                    �P�� deny_mail�Ʊ���W�@�� BAN mail���@��
                    �òΤ@�N PERM_CHAT, PERM_PAGE, PERM_POST
                    �� �۰��ܧ��v��, �Τ@�޲z, �P����ܧ��v�����O */

    level = cuser.userlevel;
    if ((level & (PERM_FORWARD | PERM_DENYMAIL)) != PERM_FORWARD)
    {
        if (level & PERM_DENYMAIL)
        {
            /*
            if ((cuser.numemail / 16U) < (cuser.numlogins + cuser.numposts))
            {
                cuser.userlevel = level ^ PERM_DENYMAIL;
                return 0;
            }
            */
            outz("�� �z���H�c�Q��F�I");
        }
        return -1;
    }
    return 0;
}
#endif

static int
xo_forward(
    XO *xo)
{
    static char rcpt[64];
    char fpath[128], folder[80], *dir, *title, *userid, ckforward[80];
    HDR *hdr, xhdr;
    int tag, locus, userno, cc, check;
    unsigned int method;                        /* �O�_ uuencode */
    ACCT acct;
    int success_count = 0;

    if (deny_forward())
        return XO_FOOT;

    /* lkchu.990428: mat patch ��ݪ��|����w�A�ץ�forward�|�_�u�����D */
    if (bbsmode == M_READA)
    {
        /* lkchu.981205: �ɥ� method �s��ݪ��ݩ� */
        method = (bshm->bcache + currbno)->battr;
        if (!HAS_PERM(PERM_SYSOP) && (method & BRD_NOFORWARD))
        {
            outz("�� ���O�峹���i��K");
            return XO_FOOT;
        }
    }

/*
    if ((hdr->xmode & POST_LOCK) && !HAS_PERM(PERM_SYSOP))
    {
        vmsg("Access Deny!");
        return XO_FOOT;
    }
*/

    tag = AskTag("��H");
    if (tag < 0)
        return XO_FOOT;

    if (!rcpt[0])
        strcpy(rcpt, cuser.email);

    if (!vget(B_LINES_REF, 0, "�ت��a�G", rcpt, sizeof(rcpt), GCARRY))
        return XO_FOOT;

    userid = cuser.userid;

    /* �Ѧ� struct.h �� MQ_UUENCODE / MQ_JUSTIFY */

#define MF_SELF 0x04
#define MF_USER 0x08

    userno = 0;
    check = 0;

    if (!mail_external(rcpt))    /* ���~�d�I */
    {
        usr_fpath(ckforward, rcpt, FN_FORWARD);
        if (!access(ckforward, 0))
        {
            if (acct_load(&acct, rcpt) >= 0)
            {
                strcpy(rcpt, acct.email);
                method = 0;
                check = 1;
            }
            else
            {
                sprintf(fpath, "�d�L���H�G%s", rcpt);
                zmsg(fpath);
                return XO_FOOT;
            }
        }
        else if (!str_cmp(rcpt, userid))
        {
            /* userno = cuser.userno; */ /* Thor.981027: �H��ﶰ���ۤv���q���ۤv */
            method = MF_SELF;

            if (mail_stat(CHK_MAIL_NOMSG))
            {
                vmsg("�A���H�c�e�q�W�L�W���A�L�k�ϥΥ��\\��I");
                chk_mailstat = 1;
                return XO_FOOT;
            }
            else
                chk_mailstat = 0;

        }
        else
        {
            if ((userno = acct_userno(rcpt)) <= 0)
            {
                sprintf(fpath, "�d�L���H�G%s", rcpt);
                zmsg(fpath);
                return XO_FOOT;
            }
            method = MF_USER;
        }

        usr_fpath(folder, rcpt, fn_dir);
    }
    else
    {
        if (not_addr(rcpt))
            return XO_FOOT;

        method = 0;

#if 0
        method = vans("�O�_�ݭn uuencode(y/N)�H[N] ") == 'y' ?
            MQ_UUENCODE : 0;
#endif
    }

    hdr = tag ? &xhdr : (HDR *) xo_pool + xo->pos - xo->top;

    dir = xo->dir;
    title = hdr->title;
    locus = cc = 0;

    do
    {
        if (tag)
            EnumTagHdr(hdr, dir, locus++);

        // check delete or not .. by statue 2000/05/18
        if (hdr->xmode & (POST_DELETE | POST_CANCEL | POST_MDELETE))
            continue;

        if ((hdr->xmode & (POST_LOCK|GEM_RESTRICT|GEM_RESERVED)) && !(HAS_PERM(PERM_ALLBOARD) || (bbstate & STAT_BOARD)))
            continue;

        if ((hdr->xmode & GEM_LOCK) && !HAS_PERM(PERM_SYSOP))
            continue;

        if (!(hdr->xmode & GEM_FOLDER)) /* �d hdr �O�_ plain text */
        {
            xo_fpath(fpath, dir, hdr);

            if (method >= MF_SELF)
            {
                HDR mhdr;

                if ((cc = hdr_stamp(folder, HDR_LINK, &mhdr, fpath)) < 0)
                    break;

                if (method == MF_SELF)
                {
                    strcpy(mhdr.owner, "[�� �� ��]");
                    mhdr.xmode = MAIL_READ | MAIL_NOREPLY;
                }
                else
                {
                    strcpy(mhdr.owner, userid);
                }
                strcpy(mhdr.nick, cuser.username);
                strcpy(mhdr.title, title);
                if ((cc = rec_add(folder, &mhdr, sizeof(HDR))) < 0)
                    break;
            }
            else
            {
                if ((cc = bsmtp(fpath, title, rcpt, method)) < 0)
                    break;
            }
            success_count++;
        }
    } while (locus < tag);

#undef  MF_SELF
#undef  MF_USER

    if (check)
        strcpy(rcpt, cuser.email);

    if (userno > 0)
        m_biff(userno);

    if (success_count == 0)
    {
        zmsg("��H���ѡC");
    }
    else
    {
        char buf[80];
        if (success_count == ((tag == 0) ? 1 : tag))
            sprintf(buf, "��H %d �g���\\�C", success_count);
        else
            sprintf(buf, "��H %d �g���\\�A%d �g���ѡC",
                success_count, ((tag == 0) ? 1 : tag) - success_count);
        zmsg(buf);
    }

    return XO_FOOT;
}

#if 0
static void
z_download(
    char *fpath)
{
    static int num = 0;         /* �y���� */
    int pid, status;
    char buf[64];

    /* Thor.0728: �� refresh�@�U, �A�ݬݷ|���|�� */

    move(b_lines, 0);
    clrtoeol();
    refresh();
    move(b_lines, 0);
    outc('\n');
    refresh();

    sprintf(buf, "tmp/%.8s.%03d", cuser.userid, ++num);
    f_cp(fpath, buf, O_TRUNC);
    if (pid = fork())
        waitpid(pid, &status, 0);
    else
    {
        execl(BINARY_SUFFIX"sz", "-a", buf, NULL);
        exit(0);
    }
    unlink(buf);
}


static int
xo_zmodem(
    XO *xo)
{
    char fpath[128], *dir;
    HDR *hdr, xhdr;
    int tag, locus;

    if (!HAS_PERM(PERM_FORWARD))
        return XO_NONE;

    tag = AskTag("Z-modem �U��");
    if (tag < 0)
        return XO_FOOT;

    if (tag)
        hdr = &xhdr;
    else
        hdr = (HDR *) xo_pool + xo->pos - xo->top;

    locus = 0;
    dir = xo->dir;

    do
    {
        if (tag)
            EnumTagHdr(hdr, dir, locus++);

        // check delete or not .. by statue 2000/05/18
        if (hdr->xmode & (POST_DELETE | POST_CANCEL | POST_MDELETE))
            continue;

        if (hdr->xmode & (POST_LOCK|GEM_LOCK|GEM_RESERVED|GEM_RESTRICT))
            continue;


        if (!(hdr->xmode & GEM_FOLDER)) /* �d hdr �O�_ plain text */
        {
            xo_fpath(fpath, dir, hdr);
            z_download(fpath);
        }
    } while (locus < tag);

    return XO_HEAD;
}
#endif  /* #if 0 */

/* ----------------------------------------------------- */
/* �峹�@�̬d�ߡB�v���]�w                                */
/* ----------------------------------------------------- */

/* 090929.cache: ²���� */
int
xo_uquery_lite(
    XO *xo)
{
    const HDR *hdr;
    const char *userid;

    hdr = (const HDR *) xo_pool + (xo->pos - xo->top);
    if (hdr->xmode & (GEM_GOPHER | POST_INCOME | MAIL_INCOME))
        return XO_NONE;

    userid = hdr->owner;
    if (strchr(userid, '.'))
        return XO_NONE;

    grayout(0, b_lines, GRAYOUT_DARK);

    move(b_lines - 8, 0);
    clrtobot();  /* �קK�e���ݯd */

    prints("\x1b[1;34m");
    outsep(b_cols, MSG_BLINE);
    prints("\n\x1b[1;33;44m \x1b[37m�峹�@�̤θ�T�d�ߡG %*s \x1b[m\n", d_cols + 56, "");
    prints("\n");
//  clrtobot();
    /* cpos = xo->pos; */               /* chuan �O�d xo->pos ���ȡA����^�s */

    my_query(userid, 2);

    move(b_lines - 1, 0);
    clrtobot();  /* �קK�e���ݯd */
    prints("\n");

    /* xo->pos = cpos; */
    return XO_HEAD;
}

int
xo_uquery(
    XO *xo)
{
    const HDR *hdr;
    const char *userid;

    hdr = (const HDR *) xo_pool + (xo->pos - xo->top);
    if (hdr->xmode & (GEM_GOPHER | POST_INCOME | MAIL_INCOME))
        return XO_NONE;

    userid = hdr->owner;
    if (strchr(userid, '.'))
        return XO_NONE;

    move(1, 0);
    clrtobot();
//  move(2, 0);
    /* cpos = xo->pos; */               /* chuan �O�d xo->pos ���ȡA����^�s */
    my_query(userid, 0);
    /* xo->pos = cpos; */
    return XO_HEAD;
}


int
xo_usetup(
    XO *xo)
{
    const HDR *hdr;
    const char *userid;
    ACCT xuser;

    if (!HAVE_PERM(PERM_SYSOP | PERM_ACCOUNTS))
        return XO_NONE;

    hdr = (const HDR *) xo_pool + (xo->pos - xo->top);
    userid = hdr->owner;
    if (strchr(userid, '.') || (acct_load(&xuser, userid) < 0))
        return XO_NONE;

    move(2, 0);
    acct_setup(&xuser, 1);
    return XO_HEAD;
}


/* ----------------------------------------------------- */
/* �D�D���\Ū                                            */
/* ----------------------------------------------------- */


#define RS_TITLE        0x001   /* author/title */
#define RS_FORWARD      0x002   /* backward */
#define RS_RELATED      0x004
#define RS_FIRST        0x008   /* find first article */
#define RS_CURRENT      0x010   /* match current read article */
#define RS_THREAD       0x020   /* search the first article */
#define RS_SEQUENT      0x040   /* sequential read */
#define RS_MARKED       0x080   /* marked article */
#define RS_UNREAD       0x100   /* unread article */

#define CURSOR_FIRST    (RS_RELATED | RS_TITLE | RS_FIRST)
#define CURSOR_NEXT     (RS_RELATED | RS_TITLE | RS_FORWARD)
#define CURSOR_PREV     (RS_RELATED | RS_TITLE)
#define RELATE_FIRST    (RS_RELATED | RS_TITLE | RS_FIRST | RS_CURRENT)
#define RELATE_NEXT     (RS_RELATED | RS_TITLE | RS_FORWARD | RS_CURRENT)
#define RELATE_PREV     (RS_RELATED | RS_TITLE | RS_CURRENT)
#define THREAD_NEXT     (RS_THREAD | RS_FORWARD)
#define THREAD_PREV     (RS_THREAD)

/* Thor: �e���mark�峹, ��K���D��������D���B�z */

#define MARK_NEXT       (RS_MARKED | RS_FORWARD | RS_CURRENT)
#define MARK_PREV       (RS_MARKED | RS_CURRENT)

typedef PAIR_T(int /* key stroke */, int /* the mapped threading op-code */) KeyMap;

#if defined __cplusplus
/* IID.20191230: Use hash table for xover thread mode op-code list */
#define HAVE_HASH_KEYMAPLIST
typedef UnorderedMapPair<KeyMap> KeyMapList;
typedef KeyMapList::const_iterator KeyMapConstIter;
#else
typedef KeyMap KeyMapList[];
typedef const KeyMap *KeyMapConstIter;
#endif


static const KeyMapList keymap =
{
    /* search title / author */

    {'"', RS_TITLE | RS_FORWARD},
    {'?', RS_TITLE},
    {'a', RS_FORWARD},
    {'A', 0},

    /* thread : currtitle */

    {'[', RS_RELATED | RS_TITLE | RS_CURRENT},
    {']', RS_RELATED | RS_TITLE | RS_FORWARD | RS_CURRENT},
    {'=', RS_RELATED | RS_TITLE | RS_FIRST | RS_CURRENT},

    /* i.e. < > : make life easier */

    {',', RS_THREAD},
    {'.', RS_THREAD | RS_FORWARD},

    /* thread : cursor */

    {'-', RS_RELATED | RS_TITLE},
    {'+', RS_RELATED | RS_TITLE | RS_FORWARD},
    {'\\', RS_RELATED | RS_TITLE | RS_FIRST},

    /* Thor: marked : cursor */
    {'\'', RS_MARKED | RS_FORWARD | RS_CURRENT},
    {';', RS_MARKED | RS_CURRENT},

    /* Thor: �V�e��Ĥ@�g��Ū���峹 */
    /* Thor.980909: �V�e�䭺�g��Ū, �Υ��g�wŪ */
    {'`', RS_UNREAD /* | RS_FIRST */},

    /* sequential */

    {' ', RS_SEQUENT | RS_FORWARD},
    {KEY_RIGHT, RS_SEQUENT | RS_FORWARD},
    {KEY_PGDN, RS_SEQUENT | RS_FORWARD},
    {KEY_DOWN, RS_SEQUENT | RS_FORWARD},
    /* Thor.990208: ���F��K�ݤ峹�L�{��, ���ܤU�g, ���M�W�h�Qxover�Y���F:p */
    {'j', RS_SEQUENT | RS_FORWARD},

    {KEY_UP, RS_SEQUENT},
    {KEY_PGUP, RS_SEQUENT},
    /* Thor.990208: ���F��K�ݤ峹�L�{��, ���ܤW�g, ���M�W�h�Qxover�Y���F:p */
    {'k', RS_SEQUENT},

    /* end of keymap */

    {0, -1}
};


GCC_PURE static int
xo_keymap(
    int key)
{
    KeyMapConstIter km;

#ifdef HAVE_HASH_KEYMAPLIST
    km = keymap.find(key);
    if (km == keymap.end())
        return -1;
#else
    int ch;

    km = keymap;
    while ((ch = km->first))
    {
        if (ch == key)
            break;
        km++;
    }
#endif

    return km->second;
}


static int
xo_thread(
    XO *xo,
    int op)
{
    static char s_author[16], s_title[32], s_unread[2]="0";
    char buf[80];

    const char *query=NULL;
    const char *tag, *title=NULL;
    int pos, match, near=0, neartop=0, max;     /* Thor: neartop�Pnear����� */

    int fd, top, bottom, step, len;
    HDR *pool;
    const HDR *fhdr;

    match = 0;
    pos = xo->pos;
    top = xo->top;
    pool = (HDR *) xo_pool;
    fhdr = pool + (pos - top);
    step = (op & RS_FORWARD) ? 1 : - 1;

    if (op & RS_RELATED)
    {
        tag = fhdr->title;

        if (op & RS_CURRENT)
        {
            query = currtitle;
            if (op & RS_FIRST)
            {
                if (!strcmp(query, tag))/* �ثe���N�O�Ĥ@���F */
                    return XO_NONE;
                near = -1;
            }
        }
        else
        {
            title = str_ttl(tag);
            if (op & RS_FIRST)
            {
                if (title == tag)
                    return XO_NONE;
                near = -1;
            }
            strcpy(buf, title);
            query = buf;
        }
    }
    else if (op & RS_UNREAD)    /* Thor: �V�e��M�Ĥ@�g��Ū�峹, �M near */
    {
#define RS_BOARD        0x1000  /* �Ω� RS_UNREAD�A��e�������i���| */
        /* Thor.980909: �߰� "���g��Ū" �� "���g�wŪ" */
        if (!vget(B_LINES_REF, 0, "�V�e��M 0)���g��Ū 1)���g�wŪ ", s_unread, sizeof(s_unread), GCARRY))
            return XO_FOOT; /* Thor.980911: ����, �h�S�MXO_FOOT, �A�ݬݫ��� */
        match |= XR_FOOT;  /* IID.20200204: Redraw footer if found */

        if (*s_unread == '0')
            op |= RS_FIRST;  /* Thor.980909: �V�e��M���g��Ū */

        near = xo->dir[0];
        if (near == 'b')                /* search board */
            op |= RS_BOARD;
        else if (near != 'u')   /* search user's mbox */
            return XO_FOOT;

        near = -1;
    }
    else if (!(op & (RS_THREAD | RS_SEQUENT | RS_MARKED)))
    {
        char *tag_query;
        if (op & RS_TITLE)
        {
            title = "���D";
            tag_query = s_title;
            len = sizeof(s_title);
        }
        else
        {
            title = "�@��";
            tag_query = s_author;
            len = sizeof(s_author);
        }
        sprintf(buf, "�j�M%s(%s)�G", title, (step > 0) ? "��" : "��");
        if (!vget(B_LINES_REF, 0, buf, tag_query, len, GCARRY))
            return XO_FOOT;
        /* Thor.980911: �n�`�N, �p�G�S���, "�j�M"���T���|�Q�M,
                        �p�G���F, �h�S�Q�M, �]�Ǧ^�Ȭ�match, �S�k�a XO_FOOT */
        match |= XR_FOOT;  /* IID.20200204: Redraw footer if found */

        str_lower(buf, tag_query);
        query = buf;
    }

    fd = -1;
    len = sizeof(HDR) * XO_TALL;
    max = xo->max;
    bottom = BMIN(top + XO_TALL, max);

    for (;;)
    {
        if (step > 0)
        {
            if (++pos >= max)
                break;
        }
        else
        {
            if (--pos < 0)
                break;
        }

        /* buffer I/O : shift sliding window scope */

        if ((pos < top) || (pos >= bottom))
        {
            if (fd < 0)
            {
                fd = open(xo->dir, O_RDONLY);
                if (fd < 0)
                    return XO_QUIT;
            }

            if (step > 0)
            {
                top += XO_TALL;
                bottom = BMIN(top + XO_TALL, max);
            }
            else
            {
                bottom = top;
                top -= XO_TALL;
            }

            lseek(fd, (off_t) (sizeof(HDR) * top), SEEK_SET);
            read(fd, pool, len);

            fhdr = (step > 0) ? pool : pool + XO_TALL - 1;
        }
        else
        {
            fhdr += step;
        }

        /* ���L�w�R�� or lock �峹 */
        if (fhdr->xmode & (POST_CANCEL | POST_DELETE | POST_MDELETE | POST_LOCK))
            continue;

        if (op & RS_SEQUENT)
        {
            match += XO_MOVE + XO_REL;
            break;
        }

        /* Thor: �e�� search marked �峹 */

        if (op & RS_MARKED)
        {
            if (fhdr->xmode & (POST_MARKED /* | POST_GEM */))
            {
                match += XO_MOVE + XO_REL;
                break;
            }
            continue;
        }

        /* �V�e��M�Ĥ@�g��Ū�峹 */

        if (op & RS_UNREAD)
        {
            /* Thor.980909: ���g��Ū(RS_FIRST) �P ���g�wŪ(!RS_FIRST) */
            if (op & RS_BOARD)
            {
                /* if (!brh_unread(fhdr->chrono)) */
//              if (!(op & RS_FIRST) ^ !brh_unread(fhdr->chrono))
                if (!(op & RS_FIRST) ^ !brh_unread(BMAX(fhdr->chrono, fhdr->stamp)))
                    continue;
            }
            else
            {
                /* if ((fhdr->xmode & MAIL_READ) */
                if (!(op & RS_FIRST) == !(fhdr->xmode & MAIL_READ))
                    continue;
            }

#undef  RS_BOARD
            /* Thor.980909: ���g�wŪ(!RS_FIRST) */
            if (!(op & RS_FIRST))
            {
                match += XO_MOVE + XO_REL;
                break;
            }

            near = pos;         /* Thor:�O�U�̱���_�I����m */
            neartop = top;
            continue;
        }

        /* ------------------------------------------------- */
        /* �H�U�j�M title / author                           */
        /* ------------------------------------------------- */

        if (op & (RS_TITLE | RS_THREAD))
        {
            title = fhdr->title;        /* title ���V [title] field */
            tag = str_ttl(title);       /* tag ���V thread's subject */

            if (op & RS_THREAD)
            {
                if (tag == title)
                {
                    match += XO_MOVE + XO_REL;
                    break;
                }
                continue;
            }
        }
        else
        {
            tag = fhdr->owner;  /* tag ���V [owner] field */
        }

        if (((op & RS_RELATED) && !strncmp(tag, query, 40)) ||
            (!(op & RS_RELATED) && str_str(tag, query)))
        {
            if (op & RS_FIRST)
            {
                if (tag != title)
                {
                    near = pos;         /* �O�U�̱���_�I����m */
                    neartop = top;
                    continue;
                }
            }

#if 0
            if ((!(op & RS_CURRENT)) && (op & RS_RELATED) &&
                strncmp(currtitle, query, TTLEN))
            {
                str_ncpy(currtitle, query, TTLEN);
                match |= XR_BODY;
            }
            else
#endif

                match += XO_MOVE + XO_REL;
            break;
        }
    }

    bottom = xo->top;

    if ((match & XO_POS_MASK) > XO_NONE)
    {
        /* A thread article is found */
        xo->pos = pos;
        if (bottom != top)
        {
            xo->top = top;
            match |= XR_BODY;           /* ���F�A�åB�ݭn��s�e�� */
        }
    }                           /* Thor: �[�W RS_FIRST�\�� */
    else if ((op & RS_FIRST) && near >= 0)
    {
        /* A thread article is found */
        match += XO_MOVE + XO_REL;
        xo->pos = near;
        if (top != neartop)             /* Thor.0609: top ���ثe��buffer��top */
        {
            lseek(fd, (off_t) (sizeof(HDR) * neartop), SEEK_SET);
            read(fd, pool, len);
        }
        if (bottom != neartop)  /* Thor.0609: bottom���e����top */
        {
            xo->top = neartop;
            match |= XR_BODY;           /* ���F�A�åB�ݭn��s�e�� */
        }
    }
    else if (bottom != top)
    {
        lseek(fd, (off_t) (sizeof(HDR) * bottom), SEEK_SET);
        read(fd, pool, len);
    }

    if (fd >= 0)
        close(fd);

    return (match & XO_POS_MASK) ? match : match + XO_NONE;
}


/* Thor.990204: ���Ҽ{more �Ǧ^��, �H�K�ݤ@�b�i�H�� []...
                ch �����emore()���ҫ���key */
int
xo_getch(
    XO *xo,
    int ch)
{
    int op;

    if (!ch)
        ch = vkey();

    op = xo_keymap(ch);
    if (op >= 0)
    {
        ch = xo_thread(xo, op);
        if ((ch & XO_POS_MASK) > XO_NONE)  /* A thread article is found */
            ch = XO_BODY;               /* �~���s�� */
    }

#if 0
    else
    {
        if (ch == KEY_LEFT || ch == 'Q')
            ch = 'q';
    }
#endif

    return ch;
}


static int
xo_jump(                        /* ���ʴ�Ш� number �Ҧb���S�w��m */
    int pos)
{
    char buf[6];

    buf[0] = pos;
    buf[1] = '\0';
    vget(B_LINES_REF, 0, "���ܲĴX���G", buf, sizeof(buf), GCARRY);
    move(b_lines, 0);
    clrtoeol();
    pos = atoi(buf);
    if (pos >= 0)
        return XR_FOOT + XO_MOVE + pos - 1;
    return XO_FOOT;
}


/* ----------------------------------------------------- */
/* Callback functions for returning special xover keys   */
/* ----------------------------------------------------- */

int xo_cb_init(XO *xo) { return XO_INIT; }
int xo_cb_load(XO *xo) { return XO_LOAD; }
int xo_cb_head(XO *xo) { return XO_HEAD; }
int xo_cb_neck(XO *xo) { return XO_NECK; }
int xo_cb_body(XO *xo) { return XO_BODY; }
int xo_cb_foot(XO *xo) { return XO_FOOT; }
int xo_cb_last(XO *xo) { return XO_LAST; }
int xo_cb_quit(XO *xo) { return XO_QUIT; }

/* ----------------------------------------------------- */
/* ----------------------------------------------------- */

XZ xz[] =
{
    {NULL, M_BOARD},      /* XZ_INDEX_CLASS */
    {NULL, M_LUSERS},     /* XZ_INDEX_ULIST */
    {NULL, M_PAL},        /* XZ_INDEX_PAL */
    {NULL, M_VOTE},       /* XZ_INDEX_VOTE */
    {NULL, M_BMW},        /* XZ_INDEX_BMW */    /* lkchu.981230: BMW �s���� */
#ifdef XZ_INDEX_XPOST /* Thor.990303: �p�G�� XZ_INDEX_XPOST���� */
    {NULL, M_READA},  /* XZ_INDEX_XPOST */
#else
    {NULL, M_READA},      /* skip XZ_INDEX_XPOST */
#endif
    {NULL, M_RMAIL},      /* XZ_INDEX_MBOX */
    {NULL, M_READA},   /* XZ_INDEX_BOARD / XZ_INDEX_POST */
    {NULL, M_GEM},        /* XZ_INDEX_GEM */
    {NULL, M_RMAIL},      /* XZ_INDEX_MAILGEM */
    {NULL, M_BANMAIL},    /* XZ_INDEX_BANMAIL */
    {NULL, M_OMENU},      /* XZ_INDEX_OTHER */
#ifdef HAVE_FAVORITE
    {NULL, M_MYFAVORITE}, /* XZ_INDEX_MYFAVORITE */
#endif
};


/* ----------------------------------------------------- */
/* interactive menu routines                             */
/* ----------------------------------------------------- */


void
xover(
    int cmd)
{
    int redo_flags = 0;  /* Collected redraw/reloading flags */
    int zone_flags = 0;  /* Collected zone operation flags */
    int pos;
    int pos_prev = -1;  /* Draw cursor on entry */
    int top_prev = -1;  /* For showing the message for the last page which is full of items */
    int wrap_flag;
    int num=0;
    int zone=-1;
    int sysmode=0;
    XO *xo=NULL;
    KeyFuncListRef xcmd=NULL;
    KeyFuncIter cb;


    /* Thor.0613: ���U�T�� */
    static int msg = 0;

    if (xo_user_level >= MAX_LEVEL)
    {
        vmsg("�w�g�W�L�̤j�h�ơA�����D�гq�� root �I");
        return;
    }

    xo_user_level++;


    for (;;)
    {
        /* Thor.0613: ���U�T���M�� */
        /* IID.20200209: `<= 0`: No messages; `>= 1`: The message will be cleared after `msg-1` loops */
        if (msg > 0)
        {
            msg--;
            if (!msg)
                cmd |= XR_FOOT;
        }

        while ((cmd != XO_NONE) || redo_flags || zone_flags)
        {
            if ((cmd & XO_POS_MASK) > XO_NONE)
            {
                /* --------------------------------------------- */
                /* calc cursor pos and show cursor correctly     */
                /* --------------------------------------------- */

                const bool zone_op = cmd & XZ_ZONE;
                const bool wrap = cmd & XO_WRAP;
                const bool rel = cmd & XO_REL;
                int cur;
                int diff;

                pos = (cmd & XO_POS_MASK) - XO_MOVE;
                cmd = (cmd & ~XO_MOVE_MASK) + XO_NONE;

                if (!xo && !zone_op)
                    continue;  /* Nothing to move */

                /* fix cursor's range */

                num = ((zone_op) ? XZ_COUNT : xo->max) - 1;
                cur = (zone_op) ? zone : xo->pos;

                if (rel)
                    pos += cur;
                diff = pos - cur;

                if (pos < 0)
                    pos = (wrap) ? num - (-pos-1) % BMAX(num, 1) : 0;
                else if (pos > num)
                    pos = (wrap) ? (pos-1) % BMAX(num, 1) : num;

                /* IID.20200129: Switch zone using cursor movement semantic */
                if (zone_op)
                {
                    /* --------------------------------------------- */
                    /* switch zone                                   */
                    /* --------------------------------------------- */

                    zone_flags |= (cmd & ~XO_MOVE_MASK);  /* Collect zone operation flags */

                    if (xz[pos].xo)
                    {
                        zone = pos;
                        xo = xz[pos].xo;
                        sysmode = xz[pos].mode;
                        xcmd = xo->cb;

                        TagNum = 0;             /* clear TagList */
                        pos_prev = -1;  /* Redraw cursor */
                        cmd = XO_INIT;
                        utmp_mode(sysmode);

                        redo_flags = 0;  /* No more redraw/reloading is needed */
                    }
                    else if (rel
                        && ((wrap && UABS(diff) <= num) || (pos > 0 && pos < num)))  /* Prevent infinity loops */
                    {
                        /* Fallback movement */
                        cmd = XO_ZONE + ((wrap) ? XO_WRAP : 0) + XO_REL + diff + ((diff > 0) ? 1 : -1);
                        continue;
                    }
                    else
                    {
                        /* Switch failed; do nothing */
                        cmd = XO_NONE;
                    }
                }
                else if (pos != cur)        /* check cursor's range */
                {
                    xo->pos = pos;
                    num = xo->top;
                    if ((pos < num) || (pos >= num + XO_TALL))
                    {
                        xo->top = (pos / XO_TALL) * XO_TALL;
                        cmd |= XR_LOAD;     /* ���J��ƨä��H��� */
                    }
                    else
                    {
                        cursor_clear(3 + cur - num, 0);
                        pos_prev = -1;  /* Redraw cursor */
                    }
                }
            }

            /* ----------------------------------------------- */
            /* Collect and adjust operation flags              */
            /* ----------------------------------------------- */

            /* Collect and strip off operation flags */
            if (cmd & XZ_ZONE)
                zone_flags |= (cmd & ~XO_MOVE_MASK);
            else
                redo_flags |= (cmd & ~XO_MOVE_MASK);
            cmd &= XO_MOVE_MASK;

            /* Process collected operation flags if there is nothing else to do */
            if (cmd == XO_NONE)
            {
                /* Process zone operation flags and then redraw/reloading flags */
                if (zone_flags)
                {
                    cmd |= zone_flags;
                    zone_flags = 0;
                }
                else if (redo_flags)
                {
                    cmd |= redo_flags;
                    redo_flags = 0;
                }
                else
                    continue;
            }

            /* ----------------------------------------------- */
            /* Special handling of operations                  */
            /* ----------------------------------------------- */

            if (cmd & XZ_ZONE)
            {
                if (cmd & XZ_QUIT)
                {
                    xo_user_level--;
                    return;
                }
            }
            else
            {
                if (cmd & XR_PART_BODY)
                {
                    pos_prev = -1;  /* Item will be redrawn; redraw cursor */
                }
                if (cmd & XR_PART_FOOT)
                {
                    move(b_lines, 0);
                    clrtoeol();
                    msg = 0;  /* Message cleared */

                    /* IID.20191223: Continue to invoke the callback function */
                }
            }

            /* ----------------------------------------------- */
            /* ���� call-back routines                         */
            /* ----------------------------------------------- */

            if (!xcmd)
            {
                /* Nothing to invoke */
                cmd = XO_NONE;
                continue;
            }

            /* IID.20191225: In C++ mode, use hash table for xover callback function list */
#if !NO_SO
            num = cmd | XO_DL; /* Thor.990220: for dynamic load */
#endif
#ifndef HAVE_HASH_KEYFUNCLIST  /* Callback function fetching loop */
            cb = xcmd;
            for (;;)
            {
                pos = cb->first;
#endif
#if !NO_SO
                /* Thor.990220: dynamic load, with key | XO_DL */
  #ifdef HAVE_HASH_KEYFUNCLIST
                cb = xcmd->find(num);
                if (cb != xcmd->end())
  #else
                if (pos == num)
  #endif
                {
                    int (*p)(XO *xo) = (int (*)(XO *xo)) DL_GET(cb->second.dlfunc);
                    if (p)
                    {
  #ifdef HAVE_HASH_KEYFUNCLIST
    #ifdef DL_HOTSWAP
                        xcmd->erase(num);
                        cb = xcmd->insert({cmd, {p}}).first;
    #endif
  #else
    #ifdef DL_HOTSWAP
                        cb->second.func = p;
                        cb->first = cmd;
    #endif
                        pos = cmd;
  #endif
                    }
                    else
                    {
                        cmd = XO_NONE;
                        goto xover_callback_end;
                    }
                }
#endif
#ifdef HAVE_HASH_KEYFUNCLIST
  #if !NO_SO
                else
  #endif
                    cb = xcmd->find(cmd);
                if (cb != xcmd->end())
#else
                if (pos == cmd)
#endif
                {
                    cmd = (*(cb->second.func)) (xo);
                    goto xover_callback_end;
                }
                else  /* Callback function not found */
#ifndef HAVE_HASH_KEYFUNCLIST
                if (pos == 'h')
#endif
                {
                    cmd = XO_NONE;
                    goto xover_callback_end;
                }

#ifndef HAVE_HASH_KEYFUNCLIST  /* Continue callback function fetching loop */
                cb++;
            }
#endif
xover_callback_end:
            ;
        } /* Thor.990220:����: end of while ((cmd != XO_NONE) || redo_flags || zone_flags) */

        if (!xo)
        {
            /* Not in a zone; exit */
            cmd = XO_QUIT;
            continue;
        }

        utmp_mode(sysmode);
        /* Thor.990220:����:�ΨӦ^�_ event handle routine �^�ӫ᪺�Ҧ� */

        pos = xo->pos;

        if (xo->max > 0)                /* Thor:�Y�O�L�F��N��show�F */
        {
            num = 3 + pos - xo->top;

            if (num != pos_prev)
            {
                cursor_show(num, 0);
                pos_prev = num;
            }

            if (xo->top != top_prev)
            {
                if (xo->top + XO_TALL == xo->max)
                {
                    outz("\x1b[44m �����ڬݥ����F! ^O^ \x1b[m");    /* Thor.0616 */
                    msg = 1;
                }
                top_prev = xo->top;
            }
        }

        cmd = vkey();
        if (!(cuser.ufo2 & UFO2_CIRCLE) && (bbsmode == M_READA))
            wrap_flag = 0;
        else
            wrap_flag = XO_WRAP;

        /* ------------------------------------------------- */
        /* switch Zone                                       */
        /* ------------------------------------------------- */
        if (cmd == Ctrl('B'))
        {
            every_B();
            cmd = XO_INIT;
            continue;
        }
        if (cmd == Ctrl('U') && zone != XZ_INDEX_ULIST)
        {
            every_U();
            cmd = XO_INIT;
            continue;
        }
        if (cmd == Ctrl('Z'))
        {
            every_Z();
            cmd = XO_INIT;
            /* cmd = XO_FOOT;*/            /* by visor : �ץ� ���D bug */
            continue;
        }

        /* ------------------------------------------------- */
        /* �򥻪���в��� routines                           */
        /* ------------------------------------------------- */

        if (cmd == KEY_LEFT || cmd == 'e' || cmd == KEY_ESC || cmd == Meta(KEY_ESC))
        {
            /* cmd = XO_LAST; *//* try to load the last XO in future */
            if (zone == XZ_INDEX_MBOX)
            {

#ifdef HAVE_MAILUNDELETE
                int deltotal;
                char fpath[256];

                if ((deltotal = mbox_check()))
                {
                    sprintf(fpath, "�� %d �ʫH��N�n�R���A�T�w�ܡH [y/N]", deltotal);
                    if (vans(fpath) == 'y')
                    {
                        usr_fpath(fpath, cuser.userid, FN_DIR);
                        hdr_prune(fpath, 0, 0, 3);
                    }
                }
#endif
                if (mail_stat(CHK_MAIL_VALID))
                {
                    vmsg("�A���H�c�e�q�W�L�W���A�о�z�I");
                    chk_mailstat = 1;
                    continue;
                }
                else
                    chk_mailstat = 0;
            }
            xo_user_level--;
            return;
        }
        else if (xo->max <= 0)  /* Thor: �L�F��h�L�k����� */
        {
            continue;
        }
        else if (cmd == KEY_UP || cmd == 'p' || cmd == 'k')
        {
            cmd = XO_MOVE + wrap_flag + XO_REL - 1;
        }
        else if (cmd == KEY_DOWN || cmd == 'n' || cmd == 'j')
        {
            cmd = XO_MOVE + wrap_flag + XO_REL + 1;
        }
        else if (cmd == ' ' || cmd == KEY_PGDN || cmd == 'N'  /*|| cmd == Ctrl('F') */)
        {                                   /* lkchu.990428: ���u�Ȯɧ��ӷ��v�� */
            if (pos == xo->max - 1)
            {
                /* Make the cursor snap to the list bottom on screen */
                cmd = XO_MOVE + wrap_flag + XO_REL + BMIN(xo->max, XO_TALL);
                if (wrap_flag)
                {
                    xo->top = 0;  /* Reset list top on screen */
                    cmd |= XR_LOAD;  /* Needs to reload */
                }
            }
            else
                cmd = XO_MOVE + XO_REL + XO_TALL;  /* Stop at the last item */
        }
        else if (cmd == KEY_PGUP || cmd == 'P' /*|| cmd == Ctrl('B')*/)
        {
            if (pos == 0 || (xo->top != 0 && pos == xo->max - 1))
                /* Make the cursor snap to the list top or bottom on screen */
                cmd = XO_MOVE + wrap_flag + XO_REL - ((xo->max-1 - xo->top) % XO_TALL + 1);
            else
                cmd = XO_MOVE + XO_REL - XO_TALL;  /* Stop at the first item */
        }
        else if (cmd == KEY_HOME || cmd == '0')
        {
            cmd = XO_MOVE;
        }
        else if (cmd == KEY_END || cmd == '$')
        {
            cmd = XR_LOAD + XO_MOVE + XO_TAIL;  /* force re-load */
        }
        else if (cmd >= '1' && cmd <= '9')
        {
            cmd = xo_jump(cmd);
        }
        else
        {
            /* ----------------------------------------------- */
            /* keyboard mapping                                */
            /* ----------------------------------------------- */

            if (cmd == KEY_RIGHT || cmd == '\n')
            {
                if (zone == XZ_INDEX_ULIST)
                    cmd = 'q'; //�ϥΪ̦W��| Q
                else
                    cmd = 'r';
            }
#ifdef XZ_INDEX_XPOST
            else if (zone >= XZ_INDEX_XPOST && zone < XZ_INDEX_BANMAIL/* XZ_INDEX_MBOX */)
#else
            else if (zone >= XZ_INDEX_MBOX && zone < XZ_INDEX_BANMAIL)
#endif
            {
                /* --------------------------------------------- */
                /* Tag                                           */
                /* --------------------------------------------- */

                if (cmd == 'C')
                {
                    cmd = xo_copy(xo);
                }
                else if (cmd == 'F')
                {
                    cmd = xo_forward(xo);
                }
#if 0
                else if (cmd == 'Z')
                {
                    cmd = xo_zmodem(xo);
                }
#endif
                else if (cmd == Ctrl('C'))
                {
                    if (TagNum)
                    {
                        TagNum = 0;
                        cmd = XO_BODY;
                    }
                    else
                        cmd = XO_NONE;
                }
                else if (cmd == Ctrl('A') || cmd == Ctrl('T') || cmd == Meta('T'))
                {
                    cmd = xo_tag(xo, cmd);
                }
                else if (cmd == Ctrl('D') && zone < XZ_INDEX_GEM)
                {
                    /* ��ذϭn�v�@�R��, �H�קK�~�� */

                    cmd = xo_prune(xo);
                }
                else if (cmd == 'g' && (bbstate & STAT_BOARD))
                { /* Thor.980806: �n�`�N�S�i�ݪ�(���w�ݪ�)��, bbstate�|�S��STAT_BOARD
                                  �����|�L�k�����峹 */
                    cmd = gem_gather(xo);               /* �����峹���ذ� */
                }
#ifdef  HAVE_MAILGEM
                else if (cmd == 'G' && HAS_PERM(PERM_MBOX))
                {
                    DL_HOTSWAP_SCOPE int (*mgp)(XO *xo) = NULL;
                    if (!mgp)
                    {
                        mgp = DL_NAME_GET("mailgem.so", mailgem_gather);
                        if (mgp)
                            cmd = (*mgp)(xo);
                        else
                            vmsg("�ʺA�s�����ѡA���p���t�κ޲z���I");
                    }
                    else
                        cmd = (*mgp)(xo);
                }
#endif
                /* --------------------------------------------- */
                /* �D�D���\Ū                                    */
                /* --------------------------------------------- */

#ifdef XZ_INDEX_XPOST
                if (zone == XZ_INDEX_XPOST)
                    continue;
#endif

                pos = xo_keymap(cmd);
                if (pos >= 0)
                {
                    cmd = xo_thread(xo, pos);

                    if ((cmd & XO_POS_MASK) > XO_NONE)
                    {
                        /* A thread article is found */
                        cursor_clear(num, 0);
                        pos_prev = -1;  /* Redraw cursor */
                        cmd = (cmd & ~XO_MOVE_MASK) + XO_NONE;
                    }
                    else
                    {                   /* Thor.0612: ��S���άO �w�g�O�F, ��Ф��Q�� */
                        outz("\x1b[44m ��S���F�C...:( \x1b[m");
                        msg = 2;  /* Clear the message after the next loop */
                    }
                }
            }
            /* ----------------------------------------------- */
            /* ��L���浹 call-back routine �h�B�z             */
            /* ----------------------------------------------- */

        } /* Thor.990220:����: end of vkey() handling */
    }
}


/* ----------------------------------------------------- */
/* Thor.0725: ctrl Z everywhere                          */
/* ----------------------------------------------------- */


#ifdef  EVERY_Z
static void
every_Z_Orig(void)
{
    int cmd;
    char select;
    screen_backup_t old_screen;

    scr_dump(&old_screen);
    cmd = 0;

    outz(MSG_ZONE_SWITCH);
    select = vkey();

#ifdef  HAVE_FAVORITE
    if (select == 'p')
    {
        outz(MSG_ZONE_ADVANCE);
        select = vkey();
        if (select != 'w' && select != 'u')
            select = ' ';
    }
#endif

    switch (select)
    {
#ifdef  HAVE_FAVORITE
        case 'f':
            scr_restore_keep(&old_screen);
            MyFavorite();
            break;
#endif  /* #ifdef  HAVE_FAVORITE */
        case 'a':
            cmd = XZ_GEM;
            break;

        case 'b':
            if (xz[XZ_POST - XO_ZONE].xo)
            {
                cmd = currbno;
                XoPost(cmd);
                cmd = XZ_POST;
                break;
            }
            // If the user has not entered any board, show the board list instead
            // Falls through

        case 'c':
            cmd = XZ_CLASS;
            break;

        case 'm':
            if (HAS_PERM(PERM_BASIC) && !HAS_PERM(PERM_DENYMAIL))
                cmd = XZ_MBOX;
            break;

        case 'u':
            cmd = XZ_ULIST;
            break;

        case 'w':
            cmd = XZ_BMW;
            break;

        default:
            break;
    }

    if (cmd)
        xover(cmd);

    scr_restore_free(&old_screen);
}

#ifdef HAVE_FAVORITE
static int
Every_Z_Favorite(void)
{
    MyFavorite();
    return 0;
}
#endif

static int
Every_Z_Xover(const void *arg)
{
    xover((int)arg);
    return 0;
}

static int
Every_Z_Board(void)
{
    if (xz[XZ_POST - XO_ZONE].xo)
    {
        xover(XZ_POST);
    }
    else
        vmsg("�|����ܬݪO");

    return 0;
}

static int
Every_Z_MBox(void)
{
    if (HAS_PERM(PERM_BASIC) && !HAS_PERM(PERM_DENYMAIL))
        xover(XZ_MBOX);
    else
        vmsg("�v�������άO�Q���v��");
    return 0;
}

static MENU menu_everyz[] =
{
#ifdef HAVE_FAVORITE
    {{Every_Z_Favorite}, PERM_VALID, POPUP_FUN,
    "Favorite �ڪ��̷R"},
#endif

    {{.funcarg = {Every_Z_Xover, (const void *)XZ_GEM}}, 0, POPUP_FUN | POPUP_ARG,
    "Gem      ��ذ�"},

    {{.funcarg = {Every_Z_Xover, (const void *)XZ_ULIST}}, 0, POPUP_FUN | POPUP_ARG,
    "Ulist    �ϥΪ̦W��"},

    {{Every_Z_Board}, 0, POPUP_FUN,
    "Post     �峹�C��"},

    {{.funcarg = {Every_Z_Xover, (const void *)XZ_CLASS}}, 0, POPUP_FUN | POPUP_ARG,
    "Class    �ݪO�C��"},

    {{Every_Z_MBox}, PERM_BASIC, POPUP_FUN,
    "Mail     �H�c"},

    {{.funcarg = {Every_Z_Xover, (const void *)XZ_BMW}}, 0, POPUP_FUN | POPUP_ARG,
    "Bmw      ���T����"},

    {{Every_Z_Screen}, 0, POPUP_FUN,
    "Screen   �ù��^��"},

    {{NULL}, 0, POPUP_QUIT,
    "Quit     ���}"},

    {{.title = "�ֳt���"}, POPUP_DO_INSTANT + 'U', POPUP_MENUTITLE,
    "�ֳt������"}
};

void
every_Z(void)
{
    int tmpmode, savemode;
    int tmpbno;
    XZ xy;

#ifdef  HAVE_CHK_MAILSIZE
 if (chk_mailstat == 1)
 {
    if (mail_stat(CHK_MAIL_VALID))
    {
        vmsg("�z���H�c�w�W�X�e�q�A�L�k�ϥΥ��\\��A�вM�z�z���H�c�I");
        return;
    }
    else
        chk_mailstat = 0;
 }
#endif

    memcpy(&xy, &(xz[XZ_OTHER - XO_ZONE]), sizeof(XZ));

    tmpbno = currbno;

    if (xo_stack_level < XO_STACK) {
        xo_stack_level++;
    } else {
        vmsg("�w�F��̤j�W�����|�Ŷ��I");
        return;
    }

    savemode = boardmode;
    tmpmode = bbsmode;

    if (cuser.ufo2 & UFO2_ORIGUI)
        every_Z_Orig();
    else
        popupmenu(menu_everyz, NULL, b_lines/2U - 4, d_cols/2U + 20);

    memcpy(&(xz[XZ_OTHER - XO_ZONE]), &xy, sizeof(XZ));

    if (tmpbno >= 0)
        XoPost(tmpbno);

    utmp_mode(tmpmode);

    xo_stack_level--;
    boardmode = savemode;

}

#endif  /* #ifdef  EVERY_Z */

void
every_U(void)
{
    int cmd, tmpmode;
    screen_backup_t old_screen;
    XZ xy;

#ifdef  HAVE_CHK_MAILSIZE
 if (chk_mailstat == 1)
 {
    if (mail_stat(CHK_MAIL_VALID))
    {
        vmsg("�z���H�c�w�W�X�e�q�A�L�k�ϥΥ��\\��A�вM�z�z���H�c�I");
        return;
    }
    else
        chk_mailstat = 0;
 }
#endif

    int tmpway = pickup_way;
    if (bbsmode == M_READA)  /* guessi.061218: �i�J�ݪO�� ^U �w�]�ƦC */
        pickup_way = 1;

    memcpy(&xy, &(xz[XZ_OTHER - XO_ZONE]), sizeof(XZ));

    cmd = XZ_ULIST;
    tmpmode = bbsmode;
    scr_dump(&old_screen);
    xover(cmd);
    scr_restore_free(&old_screen);

    memcpy(&(xz[XZ_OTHER - XO_ZONE]), &xy, sizeof(XZ));

    utmp_mode(tmpmode);

    pickup_way = tmpway; /* �٭���e�]�w���ƦC�覡 */

    return;
}

void
every_B(void)
{
    screen_backup_t old_screen;
    int tmpmode, stat;

    stat = bbstate;
    tmpmode = bbsmode;
    scr_dump(&old_screen);

    u_lock();

    scr_restore_free(&old_screen);
    bbstate = stat;

    utmp_mode(tmpmode);
    return;
}

void
every_S(void)
{
    int tmpmode;
    screen_backup_t old_screen;

    tmpmode = bbsmode;
    scr_dump(&old_screen);
    Select();
    scr_restore_free(&old_screen);
    utmp_mode(tmpmode);

    return;
}
